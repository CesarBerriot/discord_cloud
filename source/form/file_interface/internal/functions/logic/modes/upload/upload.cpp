#include "upload.hpp"
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

namespace file_interface::internal::functions::logic::modes::upload
{	struct data : types::mode_internal_data {};

	static auto get_data = std::bind(generic::get_mode_internal_data<data>, types::mode::download);
	bool draw_channel_emptiness_check();
	bool draw_directory_emptiness_check();
	void draw_upload_button();

	void draw()
	{	if
		(	internal::draw_finished_message(get_data()) &&
			internal::draw_channel_emptiness_checker_load_message() &&
			draw_channel_emptiness_check() &&
			draw_directory_emptiness_check()
		)
			draw_upload_button();
	}

	bool draw_channel_emptiness_check()
	{	if(*loaders::channel_emptiness_checker)
			return true;
		else
		{	helpers::draw_red_text("Channel can't contain messages.");
			ImGui::BeginDisabled(task::is_running());
			if(ImGui::Button("Rescan Channel"))
			{	--loaders::channel_emptiness_checker;
				loaders::channel_emptiness_checker();
			}
			ImGui::SameLine();
			if(ImGui::Button("Clear Channel"))
				task::run_safe
				(	[]
					{	if(!loaders::channel_messages_loader)
						{	logger::message(strings::loading_channel_messages.c_str());
							loaders::channel_messages_loader();
						}
						if(loaders::channel_messages_loader->size() > 1)
						{	std::vector<std::string> identifiers;
							for(nlohmann::json message : *loaders::channel_messages_loader)
								identifiers.push_back(message["id"]);
							while(!identifiers.empty())
							{	int amount = std::min((int)identifiers.size(), 100);
								decltype(identifiers) command_identifiers;
								command_identifiers.assign(identifiers.begin(), identifiers.begin() + amount);
								identifiers.erase(identifiers.begin(), identifiers.begin() + amount);
								request_handler::run
								(	std::format("channels/{}/messages/bulk-delete", variables::selected_channel),
									{},
									{ { { "messages", command_identifiers } } }
								);
							}
						}
						else
							request_handler::run
							(	std::format
								(	"channels/{}/messages/{}",
									variables::selected_channel,
									(std::string)loaders::channel_messages_loader->at(0)["id"]
								),
								{},
								{},
								{},
								"DELETE"
							);
						--loaders::channel_emptiness_checker;
						--loaders::channel_messages_loader;
					}
				);
			ImGui::EndDisabled();
			return false;
		}
	}

	bool draw_directory_emptiness_check()
	{	if(::helpers::list_directory_files_recursively(variables::directory).empty())
		{	helpers::draw_red_text("Directory can't be empty.");
			return false;
		}
		else
			return true;
	}

	void draw_upload_button()
	{	ImGui::BeginDisabled(task::is_running());
		if(ImGui::Button("Upload"))
			task::run_safe
			(	[]
				{	int splits_amount = 0;
					archiver::archive
					(	variables::directory,
						[&splits_amount](int index, archiver::split_t split)
						{	std::string name = std::to_string(index);
							logger::message(std::format("Uploading Part {}...", name));
							request_handler::run
							(	std::format("channels/{}/messages", variables::selected_channel),
								{},
								{},
								{ { name, split } }
							);
							++splits_amount;
						}
					);
					request_handler::run
					(	std::format("channels/{}/messages", variables::selected_channel),
						{},
						{ { { "content", std::to_string(splits_amount) } } }
					);
					logger::message(std::format("Uploaded {} Part(s)...", splits_amount));
					variables::mutex.lock();
					get_data().done = true;
					variables::mutex.unlock();
				}
			);
		ImGui::EndDisabled();
	}
}
