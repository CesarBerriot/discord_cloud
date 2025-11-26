#include "helpers.hpp"

namespace helpers
{	void run_safe_once(std::function<void()> procedure)
	{	if(!task::is_running())
			task::run_safe(procedure);
	}

	std::vector<std::filesystem::path> list_directory_files_recursively(std::filesystem::path path)
	{	std::vector<std::filesystem::path> result;
		for(std::filesystem::directory_entry entry : std::filesystem::recursive_directory_iterator(path))
			if(entry.is_regular_file())
				result.push_back(entry.path());
		return result;
	}

	float compute_percentage(int from, int to)
	{	return 100 * ((float)to / from);
	}

	std::filesystem::path create_temporary_directory()
	{	std::filesystem::path result = std::filesystem::temp_directory_path();
		for(int i = 0; ; ++i)
		{	std::filesystem::path new_result = result / std::to_string(i);
			if(!std::filesystem::exists(new_result))
			{	std::filesystem::create_directory(new_result);
				result = new_result;
				break;
			}
		}
		return result;
	}

	void with_temporary_directory(std::function<void(std::filesystem::path)> procedure)
	{	std::filesystem::path directory = create_temporary_directory();
		procedure(directory);
		std::filesystem::remove_all(directory);
	}
}
