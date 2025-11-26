#include "internal.hpp"
#include <imgui.h>
#include "form/file_interface/internal/strings.hpp"
#include "form/file_interface/internal/loaders/channel_emptiness_checker.hpp"
#include "form/file_interface/internal/functions/logic/generic/generic.hpp"

namespace file_interface::internal::functions::logic::modes::internal
{	bool draw_finished_message(types::mode_internal_data data)
	{	if(data.done)
		{	ImGui::Text("Done.");
			return false;
		}
		else
			return true;
	}

	bool draw_channel_emptiness_checker_load_message()
	{	if(!loaders::channel_emptiness_checker)
		{	ImGui::Text(strings::checking_channel_emptiness.c_str());
			loaders::channel_emptiness_checker();
			return false;
		}
		else
			return true;
	}
}
