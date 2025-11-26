#pragma once

#include <functional>

namespace task
{	inline std::function<bool()> is_running, is_task_thread;
	inline std::function<void(std::function<void()>)> run;
	void run_safe(std::function<void()>);
}
