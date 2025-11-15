#include <optional>
#include <map>
#include <string>
#include <nlohmann/json.hpp>

namespace request_handler
{	void draw_token_input();
	nlohmann::json run
	(	std::string path,
		std::map<std::string, std::string> options = {},
		std::optional<nlohmann::json> body = {}
	);
}