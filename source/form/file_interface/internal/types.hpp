#pragma once

#include <tuple>
#include <string>

namespace file_interface::internal::types
{	enum class mode
	{	none,
		download,
		upload,
		max
	};

	struct mode_internal_data
	{	bool done = false;
	};

	typedef
		std::tuple
		<	std::string,
			void(*)(),
			mode_internal_data*
		>
		mode_data_t;
}
