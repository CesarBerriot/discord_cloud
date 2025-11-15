#include "request_handler/request_handler.hpp"
#include "task/task.hpp"

namespace helpers
{	template<typename... argument_types>
	void run_safe_request_once_then
	(	std::function<void(decltype(request_handler::run({})))> procedure,
		argument_types... arguments
	)
	{	if(!task::is_running())
			task::run_safe
			(	[procedure, arguments...]
				{	procedure(request_handler::run(arguments...));
				}
			);
	}
}