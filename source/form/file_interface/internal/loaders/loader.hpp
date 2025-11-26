#pragma once

#include <optional>
#include <mutex>
#include <memory>
#include <string>
#include <stdexcept>
#include <format>
#include "helpers/helpers.hpp"

namespace file_interface::internal::loaders
{	template<typename type>
	class loader
	{	protected:
			std::optional<type> result;
			std::mutex mutex;
		public:
			type operator*()
			{	check_accessibility();
				std::lock_guard guard(mutex);
				return *result;
			}
			std::unique_ptr<type> operator->()
			{	check_accessibility();
				std::lock_guard guard(mutex);
				return std::make_unique<type>(*result);
			}
			operator bool()
			{	std::lock_guard guard(mutex);
				return result.has_value();
			}
			void operator--()
			{	mutex.lock();
				result.reset();
				mutex.unlock();
			}
			virtual void operator()()
			{	if(task::is_task_thread())
					load();
				else
					helpers::run_safe_once(std::bind(&loader::load, this));
			}
		protected:
			virtual std::string get_name() = 0;
			virtual void load() = 0;
		private:
			void check_accessibility()
			{	if(!*this)
					throw
						std::runtime_error
						(	std::format
							(	"Loader '{}' accessed without being done loading its data.",
								get_name()
							)
						);
			}
	};
}
