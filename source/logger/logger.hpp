#include <string>
#include <vector>
#include <functional>

namespace logger
{	void push(bool severe, std::string message);
	inline auto const
		message = std::bind(push, false, std::placeholders::_1),
		warn = std::bind(push, true, std::placeholders::_1);
	std::vector<std::pair<bool, std::string>> pop();
}