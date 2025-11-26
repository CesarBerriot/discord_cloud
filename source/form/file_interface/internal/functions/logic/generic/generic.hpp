#pragma once

#include <string>
#include "form/file_interface/internal/types.hpp"

namespace file_interface::internal::functions::logic::generic
{	bool draw_directory_input();
	bool draw_mode_selection();
	types::mode_data_t & get_mode_data(types::mode);
	template<typename data_type>
	data_type & get_mode_internal_data(types::mode mode)
	{	static_assert(std::is_base_of_v<types::mode_internal_data, data_type>);
		types::mode_internal_data *& data = std::get<2>(get_mode_data(mode));
		if(!data)
			data = new data_type;
		return *(data_type*)data;
	}
	void draw_mode_reset_button(std::string label, types::mode);
}
