#include "logger.hpp"
#include <type_traits>
#include <mutex>

namespace logger
{	typedef std::invoke_result_t<decltype(*pop)> log_t;
	log_t log;
	std::mutex mutex;
	void push(bool severe, std::string message)
	{	mutex.lock();
		log.push_back({ severe, message });
		mutex.unlock();
	}
	log_t pop()
	{	mutex.lock();
		log_t copy = log;
		log.clear();
		mutex.unlock();
		return copy;
	}
}
