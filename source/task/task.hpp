#include <functional>

namespace task
{	inline std::function<bool()> is_running;
	inline std::function<void(std::function<void()>)> run;
	void run_safe(std::function<void()>);
}
