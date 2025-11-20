#include "task.hpp"
#include "logger/logger.hpp"

void task::run_safe(std::function<void()> procedure)
{	run
	(	[procedure]
		{	try
			{	procedure();
			}
			catch(std::exception & exception)
			{	logger::warn(exception.what());
			}
		}
	);
}
