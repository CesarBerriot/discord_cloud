#include "file_interface.hpp"
#include <imgui.h>
#include "form/channel_selector/channel_selector.hpp"
#include "internal/functions/logic/generic/generic.hpp"
#include "internal/loaders/channel_emptiness_checker.hpp"
#include "internal/loaders/channel_messages_loader.hpp"

using namespace file_interface::internal;

namespace file_interface
{	void draw()
	{	variables::mutex.lock();
		variables::selected_channel = *channel_selector::get();
		if(functions::logic::generic::draw_directory_input())
			if(functions::logic::generic::draw_mode_selection())
			{	ImGui::BeginDisabled(task::is_running());
				if(ImGui::Button("Change Mode"))
					clear();
				ImGui::EndDisabled();
				if(variables::selected_mode != types::mode::none)
					std::get<1>(functions::logic::generic::get_mode_data(variables::selected_mode))();
			}
		variables::mutex.unlock();
	}

	void clear()
	{	variables::selected_mode = types::mode::none;
		for(int i = (int)types::mode::none; i < (int)types::mode::max; ++i)
		{	types::mode_internal_data *& data = std::get<2>(functions::logic::generic::get_mode_data((types::mode)i));
			delete data;
			data = NULL;
		}
		--loaders::channel_emptiness_checker;
		--loaders::channel_messages_loader;
	}
}
