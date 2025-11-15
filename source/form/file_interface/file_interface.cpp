#include "file_interface.hpp"
#include <mutex>
#include <format>
#include <memory>
#include <minwindef.h>
#include <imgui.h>
#include "form/channel_selector/channel_selector.hpp"
#include "helpers/helpers.hpp"
#include "logger/logger.hpp"

enum class mode
{	none,
	download,
	upload,
	max
};

typedef
	std::tuple
	<	std::string,
		void(*)(),
		void*
	>
	mode_data_t;

static std::string
	selected_channel,
	bot_user;
static char directory[MAX_PATH] = "";
static mode selected_mode;
static std::mutex mutex;

static bool clear_check();
static void clear();
static void draw_red_text(std::string);
static bool draw_directory_input();
static bool draw_mode_selection();
static bool load();
static mode_data_t & get_mode_data(mode);
static void draw_download_interface();
static void draw_upload_interface();


//static std::map<mode, std::function<void()>> const mode_procedures =

void draw_file_interface()
{	mutex.lock();
	selected_channel = *channel_selector::get();
	if(clear_check())
		clear();
	if(draw_directory_input())
		if(draw_mode_selection())
		;
	/*	if(load())
			ImGui::Text("Interface Placeholder");*/
	mutex.unlock();
}

bool clear_check()
{	static std::string last;
	bool result = selected_channel != last;
	last = selected_channel;
	return result;
}

void clear()
{	bot_user.clear();
	selected_mode = mode::none;
}

void draw_red_text(std::string message)
{	ImGui::TextColored({ 1, 0, 0, 1 }, "%s", message.c_str());
}

bool draw_directory_input()
{	ImGui::Text("Directory");
	ImGui::SameLine();
	ImGui::InputText("##directory input", directory, MAX_PATH);
	if(std::filesystem::is_directory(directory))
		return true;
	else
	{	draw_red_text("A valid directory path is required.");
		return false;
	}
}

bool draw_mode_selection()
{	if(selected_mode == mode::none)
	{	ImGui::Text("Mode");
		ImGui::SameLine();
		if(ImGui::BeginCombo("##mode combo", std::get<0>(get_mode_data(mode::none)).c_str()))
		{	for(int i = (int)mode::none + 1; i < (int)mode::max; ++i)
				if(ImGui::Selectable(std::get<0>(get_mode_data((mode)i)).c_str()))
					selected_mode = (mode)i;
			ImGui::EndCombo();
		}
	}
	return selected_mode != mode::none;
}

bool load()
{	if(false)
		return true;
	else
	{	if(bot_user.empty())
		{	ImGui::Text("Loading Bot User Identifier...");
			helpers::run_safe_request_once_then
			(	[](nlohmann::json data)
				{	mutex.lock();
					bot_user = data["id"];
					mutex.unlock();
				},
				"users/@me"
			);
		}
		else if(true)
			helpers::run_safe_request_once_then
			(	[](nlohmann::json data)
				{	logger::message(data.dump());
					for(auto item : data.items())
						logger::message
						(	std::format
							(	"---\n"
								"author : {}\n"
								"content : {}\n"
								"raw : \n{}",
								(std::string)item.value()["author"]["id"],
								(std::string)item.value()["content"],
								item.value().dump()
							)
						);
					logger::message("DONE");
				},
				std::format("channels/{}/messages", selected_channel)
			);
		return false;
	}
}

mode_data_t & get_mode_data(mode value)
{	static std::map<mode, mode_data_t> map =
		{	{ mode::none, mode_data_t{ "None", NULL, NULL } },
			{ mode::download, { "Download", draw_download_interface, NULL } },
			{ mode::upload, { "Upload", draw_upload_interface, NULL } }
		};
	return map.at(value);
}

void draw_download_interface()
{
}

void draw_upload_interface()
{
}
