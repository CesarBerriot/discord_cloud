#pragma once

#include <string>
#include <mutex>
#include <minwindef.h>
#include "types.hpp"

namespace file_interface::internal::variables
{	inline std::string selected_channel;
	inline char directory[MAX_PATH] = "";
	inline types::mode selected_mode;
	inline std::mutex mutex;
}
