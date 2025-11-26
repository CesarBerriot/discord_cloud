#include <filesystem>
#include <functional>
#include <optional>
#include "types.hpp"

namespace archiver
{	typedef blob_t split_t;
	void archive(std::filesystem::path, std::function<void(int, split_t)>);
	void unarchive(std::filesystem::path destination, std::function<std::optional<split_t>()> get_next_split);
}
