#include "generic.hpp"
#include <map>
#include <filesystem>
#include <imgui.h>
#include "form/file_interface/file_interface.hpp"
#include "form/file_interface/internal/types.hpp"
#include "form/file_interface/internal/variables.hpp"
#include "form/file_interface/internal/functions/helpers/helpers.hpp"
#include "form/file_interface/internal/functions/logic/modes/download/download.hpp"
#include "form/file_interface/internal/functions/logic/modes/upload/upload.hpp"
#include "task/task.hpp"

namespace file_interface::internal::functions::logic::generic
{	bool draw_directory_input()
	{	ImGui::Text("Directory");
		ImGui::SameLine();
		ImGui::BeginDisabled(task::is_running());
		ImGui::InputText("##directory input", variables::directory, MAX_PATH);
		ImGui::EndDisabled();
		if(std::filesystem::is_directory(variables::directory))
			return true;
		else
		{	helpers::draw_red_text("A valid directory path is required.");
			return false;
		}
	}

	bool draw_mode_selection()
	{	if(variables::selected_mode == types::mode::none)
		{	ImGui::Text("Mode");
			ImGui::SameLine();
			if(ImGui::BeginCombo("##mode combo", std::get<0>(get_mode_data(types::mode::none)).c_str()))
			{	for(int i = (int)types::mode::none + 1; i < (int)types::mode::max; ++i)
					if(ImGui::Selectable(std::get<0>(get_mode_data((types::mode)i)).c_str()))
						variables::selected_mode = (types::mode)i;
				ImGui::EndCombo();
			}
		}
		return variables::selected_mode != types::mode::none;
	}

	types::mode_data_t & get_mode_data(types::mode value)
	{	static std::map<types::mode, types::mode_data_t> map =
			{	{ types::mode::none, { "None", NULL, NULL } },
				{ types::mode::download, { "Download", modes::download::draw, NULL } },
				{ types::mode::upload, { "Upload", modes::upload::draw, NULL } }
			};
		return map.at(value);
	}

	void draw_mode_reset_button(std::string label, types::mode mode)
	{	if(ImGui::Button(label.c_str()))
		{	clear();
			variables::selected_mode = mode;
		}
	}
}
