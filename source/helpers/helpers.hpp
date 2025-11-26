#pragma once

#include "request_handler/request_handler.hpp"
#include <filesystem>
#include <functional>
#include "task/task.hpp"

namespace helpers
{	void run_safe_once(std::function<void()>);
	template<typename... argument_types>
	void run_safe_request_once(std::function<void(nlohmann::json)> procedure, argument_types... arguments)
	{	run_safe_once([=] { procedure(request_handler::run(arguments...)); });
	}
	std::vector<std::filesystem::path> list_directory_files_recursively(std::filesystem::path);
	float compute_percentage(int from, int to);
	std::filesystem::path create_temporary_directory();
	void with_temporary_directory(std::function<void(std::filesystem::path)>);
}