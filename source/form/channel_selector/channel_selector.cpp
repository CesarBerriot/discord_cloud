#include "channel_selector.hpp"
#include <mutex>
#include <vector>
#include <format>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include "task/task.hpp"
#include "request_handler/request_handler.hpp"

struct named_identifier { std::string name, identifier; };
typedef std::vector<named_identifier> named_identifiers_t;
typedef std::optional<named_identifier> optional_named_identifier_t;
static named_identifiers_t guilds, channels;
static optional_named_identifier_t selected_guild, selected_channel;
static std::mutex mutex;

static void draw_selection_reset_button();
static bool draw_guild_selection();
static bool draw_channel_selection();
static void draw_selector(std::string label, named_identifiers_t, optional_named_identifier_t&);

namespace channel_selector
{	std::optional<std::string> get()
	{	mutex.lock();
		std::optional<std::string> result =
			selected_channel
				? std::optional(selected_channel.value().identifier)
				: std::nullopt;
		mutex.unlock();
		return result;
	}

	bool draw()
	{	mutex.lock();
		draw_selection_reset_button();
		bool result =
			draw_guild_selection() &&
			draw_channel_selection();
		mutex.unlock();
		return result;
	}
}

void draw_selection_reset_button()
{	if(selected_guild)
	{	ImGui::BeginDisabled(task::is_running());
		if(ImGui::Button("Reset Selection"))
		{	guilds.clear();
			selected_guild.reset();
			channels.clear();
			selected_channel.reset();
		}
		ImGui::EndDisabled();
	}
}

bool draw_guild_selection()
{	if(selected_guild)
		return true;
	else
	{	if(guilds.empty())
		{	ImGui::Text("Loading Guilds...");
			if(!task::is_running())
				task::run_safe
				(	[]
					{	nlohmann::json data = request_handler::run("users/@me/guilds");
						mutex.lock();
						for(auto item : data.items())
							guilds.push_back
							(	{	.name = item.value()["name"],
									.identifier = item.value()["id"]
								}
							);
						mutex.unlock();
					}
				);
		}
		else
			draw_selector("Guild", guilds, selected_guild);
		return false;
	}
}

bool draw_channel_selection()
{	if(selected_channel)
		return true;
	else
	{	if(channels.empty())
		{	ImGui::Text("Loading Text Channels...");
			if(!task::is_running())
				task::run_safe
				(	[]
					{	nlohmann::json data = request_handler::run(std::format("guilds/{}/channels", selected_guild->identifier));
						mutex.lock();
						for(auto item : data.items())
							if(item.value()["type"] == 0)
								channels.push_back
								(	{	.name = item.value()["name"],
										.identifier = item.value()["id"]
									}
								);
						mutex.unlock();
					}
				);
		}
		else
			draw_selector("Channel", channels, selected_channel);
		return false;
	}
}

void draw_selector(std::string label, named_identifiers_t container, optional_named_identifier_t & selection)
{	ImGui::Text(("Select " + label).c_str());
	ImGui::SameLine();
	if(ImGui::BeginCombo("##combo", "None"))
	{	for(named_identifier item : container)
		if(ImGui::Selectable(std::format("{} ({})", item.name, item.identifier).c_str()))
			selection = item;
		ImGui::EndCombo();
	}
}
