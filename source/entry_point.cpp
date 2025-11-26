#include <dial/entry_point.hpp>
#include <imgui.h>
#include "form/form.hpp"

DIAL_ENTRY_POINT
(	[]
	{	ImGui::GetIO().IniFilename = NULL;
		instantiate_form();
		return true;
	}
)