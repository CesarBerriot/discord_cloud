#pragma once

#include "loader.hpp"
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include "form/file_interface/internal/variables.hpp"
#include "request_handler/request_handler.hpp"

namespace file_interface::internal::loaders
{	class channel_messages_loader : public loader<std::vector<nlohmann::json>>
	{	private:
			std::string get_name() override { return "channel messages loader"; }
			void load() override
			{	decltype(result)::value_type messages;
				enum { limit = 100 };
				for(;;)
				{	std::map<std::string, std::string> options =
						{	{ "limit", std::to_string(limit) }
						};
					if(!messages.empty())
						options["before"] = messages.back()["id"];
					nlohmann::json batch =
						request_handler::run
						(	std::format
							(	"channels/{}/messages",
								variables::selected_channel
							),
							options
						);
					if(batch.empty())
						break;
					for(nlohmann::detail::iteration_proxy_value message_item : batch.items())
						messages.push_back(message_item.value());
				}
				mutex.lock();
				result = messages;
				mutex.unlock();
			}
	};
	inline channel_messages_loader channel_messages_loader;
}
