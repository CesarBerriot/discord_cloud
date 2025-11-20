#include "archiver.hpp"
#include <cstdint>
#include <fstream>
#include <format>
#include <minwindef.h>
#include "helpers/helpers.hpp"
#include "logger/logger.hpp"
#include "compressor/compressor.hpp"

static std::string const signature = "{2A2047FA-4F97-4444-86FA-63FFB082BC14}";
static int split_size = 8 * pow(1024, 2);

namespace archiver
{	splits_t archive(std::filesystem::path path)
	{	std::vector<std::filesystem::path> files = helpers::list_directory_files_recursively(path);
		class
		{	private:
				blob_t data;
			public:
				void push_range(void * begin, void * end, bool back = true)
				{	data.insert
					(	back ? data.end() : data.begin(),
						(byte*)begin,
						(byte*)end
					);
				}
				void push_raw(void * address, size_t size, bool back = true)
				{	push_range(address, ((byte*)address) + size, back);
				}
				blob_t operator*() { return data; }
				std::unique_ptr<blob_t> operator->() { return std::make_unique<blob_t>(data); }
		} fancy_blob;
		for(std::filesystem::path file : files)
		{	logger::message(std::format("Archiving File '{}'...", file.string()));
			std::string relative_file_path = std::filesystem::relative(file, path).string();
			char path[MAX_PATH];
			std::copy(relative_file_path.begin(), relative_file_path.end(), path);
			path[relative_file_path.size()] = '\0';
			std::ifstream stream(file, std::ios::binary);
			stream.seekg(0, std::ios::end);
			int64_t size = stream.tellg();
			stream.seekg(0, std::ios::beg);
			std::unique_ptr data = std::make_unique<byte[]>(size);
			stream.read((char*)data.get(), size);
			fancy_blob.push_raw(path, MAX_PATH);
			fancy_blob.push_raw(&size, sizeof(size));
			fancy_blob.push_raw(data.get(), size);
		}
		for(int i = 0; i < 2; ++i)
			fancy_blob.push_raw((byte*)signature.data(), signature.size(), i);
		int32_t uncompressed_size = fancy_blob->size();
		blob_t blob = compressor::compress(*fancy_blob);
		blob.insert
		(	blob.begin(),
			(byte*)&uncompressed_size,
			(byte*)&uncompressed_size + sizeof(uncompressed_size)
		);
		splits_t splits;
		int splits_amount = ceil((float)blob.size() / split_size);
		byte * max_end = blob.data() + blob.size();
		for(int i = 0; i < splits_amount; ++i)
		{	byte * begin = blob.data() + split_size * i;
			byte * end = begin + split_size;
			end = std::min(end, max_end);
			splits.push_back({});
			splits_t::value_type & split = splits.back();
			split.insert(split.end(), begin, end);
		}
		return splits;
	}

	void unarchive(std::filesystem::path path, splits_t splits)
	{	blob_t blob;
		for(split_t split : splits)
			blob.insert(blob.end(), split.begin(), split.end());
		int32_t uncompressed_size = *(int32_t*)blob.data();
		blob.erase(blob.begin(), blob.begin() + sizeof(int32_t));
		blob = compressor::decompress(blob, uncompressed_size);
		int cursor = 0;
		auto read_array =
			[blob, &cursor]<typename type>(int length)
			{	type * result = (type*)(blob.data() + cursor);
				cursor += sizeof(type) * length;
				return result;
			};
		auto read =
			[blob, &cursor]<typename type>
			{	type & result = *(type*)(blob.data() + cursor);
				cursor += sizeof(type);
				return result;
			};
		auto signature_check =
			[read_array]
			{	byte * blob = read_array.operator()<byte>(signature.size());
				if(!std::equal(signature.begin(), signature.end(), blob))
					throw std::runtime_error("Signature check failure.");
			};
		signature_check();
		while(cursor < (blob.size() - signature.size()))
		{	char * file_path = read_array.operator()<char>(MAX_PATH);
			int64_t size = read.operator()<int64_t>();
			byte * data = read_array.operator()<byte>(size);
			logger::message
			(	std::format
				(	"Unarchiving {}-bytes file '{}'...",
					size,
					file_path
				)
			);
			std::filesystem::path absolute_file_path = path / file_path;
			std::function<void(std::filesystem::path)> create_parent_directory =
				[&create_parent_directory](std::filesystem::path path)
				{	if(path.has_parent_path() && path != path.root_path())
					{	std::filesystem::path parent_path = path.parent_path();
						create_parent_directory(parent_path);
						if(!std::filesystem::exists(parent_path))
							std::filesystem::create_directory(parent_path);
					}
				};
			create_parent_directory(absolute_file_path);
			std::ofstream stream(absolute_file_path, std::ios::binary);
			stream.exceptions(std::ios::failbit | std::ios::badbit);
			stream.write((char*)data, size);
		}
		signature_check();
	}
}
