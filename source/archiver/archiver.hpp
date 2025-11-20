#include <filesystem>
#include "types.hpp"

namespace archiver
{	typedef std::vector<blob_t> splits_t;
	typedef splits_t::value_type split_t;
	splits_t archive(std::filesystem::path);
	void unarchive(std::filesystem::path, splits_t);
}
