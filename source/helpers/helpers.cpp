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
}
