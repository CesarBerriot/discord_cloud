#include "helpers.hpp"
#include <imgui.h>

namespace file_interface::internal::functions::helpers
{	void draw_red_text(std::string message)
	{	ImGui::TextColored({ 1, 0, 0, 1 }, "%s", message.c_str());
	}
}
