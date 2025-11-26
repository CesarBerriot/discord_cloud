#pragma once

#include "loader.hpp"
#include <nlohmann/json.hpp>
#include "form/file_interface/internal/variables.hpp"
#include "request_handler/request_handler.hpp"

namespace file_interface::internal::loaders
{	class channel_emptiness_checker : public loader<bool>
	{	private:
			std::string get_name() override { return "channel emptiness checker"; }
			void load() override
			{	nlohmann::json data =
					request_handler::run
					(	std::format
						(	"channels/{}/messages",
							variables::selected_channel
						),
						{ { "limit", "1" } }
					);
				mutex.lock();
				result = data.empty();
				mutex.unlock();
			}
	};
	inline channel_emptiness_checker channel_emptiness_checker;
}
