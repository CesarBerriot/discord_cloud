#include "download.hpp"
#include <optional>
#include <map>
#include <imgui.h>
#include "form/file_interface/internal/strings.hpp"
#include "form/file_interface/internal/types.hpp"
#include "form/file_interface/internal/loaders/channel_emptiness_checker.hpp"
#include "form/file_interface/internal/loaders/channel_messages_loader.hpp"
#include "form/file_interface/internal/functions/helpers/helpers.hpp"
#include "form/file_interface/internal/functions/logic/generic/generic.hpp"
#include "form/file_interface/internal/functions/logic/modes/internal/internal.hpp"
#include "logger/logger.hpp"
#include "archiver/archiver.hpp"

namespace file_interface::internal::functions::logic::modes::download
{	struct data : types::mode_internal_data
	{	std::optional<int> splits_amount;
		std::map<int, std::string> split_file_links;
		std::optional<bool> load_result;
	};

	static auto get_data = std::bind(generic::get_mode_internal_data<data>, types::mode::download);
	static bool draw_channel_emptiness_check();
	static bool draw_message_loader_message();
	static bool draw_data_load_message();
	static bool draw_data_validity_check();
	static bool draw_directory_emptiness_check();
	static void draw_download_button();

	void draw()
	{	if
		(	internal::draw_finished_message(get_data()) &&
			internal::draw_channel_emptiness_checker_load_message() &&
			draw_channel_emptiness_check() &&
			draw_message_loader_message() &&
			draw_data_load_message() &&
			draw_data_validity_check() &&
			(	task::is_running() ||
				draw_directory_emptiness_check()
			)
		)
			draw_download_button();
	}

	bool draw_channel_emptiness_check()
	{	if(*loaders::channel_emptiness_checker)
		{	helpers::draw_red_text("Channel shouldn't be empty.");
			return false;
		}
		else
			return true;
	}

	bool draw_message_loader_message()
	{	if(!loaders::channel_messages_loader)
		{	ImGui::Text(strings::loading_channel_messages.c_str());
			loaders::channel_messages_loader();
			return false;
		}
		else
			return true;
	}

	bool draw_data_load_message()
	{	data & data = get_data();
		if
		(	!data.load_result ||
			(!*data.load_result && task::is_running())
		)
		{	ImGui::Text("Loading Message Data...");
			::helpers::run_safe_once
			(	[&data]
				{	variables::mutex.lock();
					data.load_result = false;
					variables::mutex.unlock();
					logger::message("Loading Bot User Identifier...");
					std::string bot_user_identifier = request_handler::run("users/@me")["id"];
					logger::message(std::format("Processing {} Messages...", loaders::channel_messages_loader->size()));
					for(nlohmann::json message : *loaders::channel_messages_loader)
					{	logger::message(std::format("Processing Message '{}'...", (std::string)message["id"]));
						if(message["author"]["id"] != bot_user_identifier)
							logger::warn("Message wasn't authored by this bot. Processing anyway.");
						if(!((std::string)message["content"]).empty())
						{	std::lock_guard guard(variables::mutex);
							if(data.splits_amount)
								throw std::runtime_error("Found multiple non-file messages.");
							else
								try { data.splits_amount = std::stoi((std::string)message["content"]); }
								catch(std::invalid_argument) { throw std::runtime_error("Message content is not a number."); }
						}
						else
						{	nlohmann::json attachments = message["attachments"];
							if(attachments.size() != 1)
								throw
									std::runtime_error
									(	std::format
										(	"Message has {} attachments, expected 1.",
											attachments.size()
										)
									);
							else
							{	std::string
									name = attachments[0]["filename"],
									link = attachments[0]["url"];
								int index;
								try { index = std::stoi(name); }
								catch(std::invalid_argument)
								{	throw
										std::runtime_error
										(	std::format
											(	"File name '{}' is not a number.",
												name
											)
										);
								}
								std::lock_guard guard(variables::mutex);
								if(data.split_file_links.contains(index))
									throw std::runtime_error(std::format("Part {} has duplicates.", index));
								else
									data.split_file_links[index] = link;
							}
						}
					}
					variables::mutex.lock();
					data.load_result = true;
					variables::mutex.unlock();
				}
			);
			return false;
		}
		else
			return true;
	}

	bool draw_data_validity_check()
	{	data & data = get_data();
		std::optional<std::string> error;
		if(!*data.load_result)
			error = "Failed to load messages. Read the log for further information.";
		else if(!data.splits_amount)
			error = "Missing splits amount message.";
		else if(data.splits_amount < 1)
			error = std::format("Splits amount can't be lower than 1.");
		else if
		(	std::vector<int> missing_splits =
				[&data]
				{	std::vector<int> result;
					for(int i = 1; i <= *data.splits_amount; ++i)
						if(!data.split_file_links.contains(i))
							result.push_back(i);
					return result;
				}();
			!missing_splits.empty()
		)
		{	std::stringstream message;
			message << "Part files missing : " << missing_splits[0];
			for(int i = 1; i < missing_splits.size(); ++i)
				message << ", " << missing_splits[i];
			error = message.str();
		}
		if(error)
		{	helpers::draw_red_text(*error);
			generic::draw_mode_reset_button("Retry", types::mode::download);
			return false;
		}
		else
			return true;
	}

	bool draw_directory_emptiness_check()
	{	if(!::helpers::list_directory_files_recursively(variables::directory).empty())
		{	helpers::draw_red_text("This mode requires an empty directory.");
			ImGui::BeginDisabled(task::is_running());
			if(ImGui::Button("Clear Directory"))
				task::run_safe
				(	[]
					{	for(std::filesystem::directory_entry entry : std::filesystem::directory_iterator(variables::directory))
							if(entry.is_directory())
								std::filesystem::remove_all(entry);
							else
								std::filesystem::remove(entry);
					}
				);
			ImGui::EndDisabled();
			return false;
		}
		else
			return true;
	}

	void draw_download_button()
	{	ImGui::BeginDisabled(task::is_running());
		if(ImGui::Button(std::format("Download ({} Parts)", *get_data().splits_amount).c_str()))
			::helpers::run_safe_once
			(	[]
				{	variables::mutex.lock();
					std::filesystem::path directory = variables::directory;
					data data = get_data();
					variables::mutex.unlock();
					int progress = 0;
					archiver::unarchive
					(	directory,
						[&progress, data]() -> std::optional<archiver::split_t>
						{	++progress;
							if(progress > *data.splits_amount)
								return std::nullopt;
							std::string link = data.split_file_links.at(progress);
							logger::message
							(	std::format
								(	"Downloading part {} ({}%)...",
									progress,
									::helpers::compute_percentage(*data.splits_amount, progress)
								)
							);
							return request_handler::download(link);
						}
					);
					variables::mutex.lock();
					get_data().done = true;
					variables::mutex.unlock();
					/*variables::mutex.lock();
					data & data = get_data();
					variables::mutex.unlock();
					archiver::splits_t splits;
					variables::mutex.lock();
					for(int i = 1; i <= *data.splits_amount; ++i)
					{	std::string link = data.split_file_links.at(i);
						variables::mutex.unlock();
						logger::message
						(	std::format
							(	"Downloading part {} ({}%)...",
								i,
								::helpers::compute_percentage(*data.splits_amount, i)
							).c_str());
						splits.push_back(request_handler::download(link));
						variables::mutex.lock();
					}
					variables::mutex.unlock();
					logger::message("Unarchiving Parts...");
					archiver::unarchive(variables::directory, splits);
					variables::mutex.lock();
					data.done = true;
					variables::mutex.unlock();*/
				}
			);
		ImGui::EndDisabled();
	}
}
