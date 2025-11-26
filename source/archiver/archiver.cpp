#include "archiver.hpp"
#include <cstdint>
#include <fstream>
#include <format>
#include <minwindef.h>
#include "helpers/helpers.hpp"
#include "logger/logger.hpp"
#include "compressor/compressor.hpp"

typedef uint64_t encoded_size_t;

#pragma pack(push, 1)

struct split_header
{	encoded_size_t
		compressed_size,
		uncompressed_size;
};

struct file_header
{	char path[MAX_PATH];
	encoded_size_t size;
};

#pragma pack(pop)

static struct
{	std::string archive_file_name = "archive";
	encoded_size_t split_size = 8 * pow(1024, 2);
} const constants;

namespace archiver
{	void archive(std::filesystem::path path, std::function<void(int, split_t)> procedure)
	{	logger::message(std::format("Archiving directory '{}'...", path.string()));

		int split_index = 0;
		split_t buffer;

		auto write_split =
			[](split_t & split, auto * data, encoded_size_t size, bool begin = false)
			{	split.insert
				(	begin ? split.begin() : split.end(),
					(byte*)data,
					(byte*)data + size
				);
			};

		auto process_buffer =
			[&split_index, &buffer, write_split, procedure](bool force = false)
			{	if
				(	!buffer.empty() &&
					(	buffer.size() >= (constants.split_size - sizeof(split_header))
						|| force
					)
				)
				{	split_header header =
					{	.uncompressed_size = std::min<encoded_size_t>(constants.split_size, buffer.size())
					};
					split_t uncompressed_split;
					uncompressed_split.assign(buffer.begin(), buffer.begin() + header.uncompressed_size);
					buffer.erase(buffer.begin(), buffer.begin() + header.uncompressed_size);
					split_t compressed_split = compressor::compress(uncompressed_split);
					header.compressed_size = compressed_split.size();
					bool compressed = header.compressed_size <= header.uncompressed_size;
					if(!compressed)
						logger::warn("Compressed split is larger than uncompressed (file is likely already compressed), uploading uncompressed.");
					split_t split = compressed ? compressed_split : uncompressed_split;
					write_split(split, &header, sizeof(header), true);
					++split_index;
					procedure(split_index, split);
				}
			};

		auto write_buffer =
			[&buffer, write_split, process_buffer](auto * data, encoded_size_t size, bool begin = false)
			{	write_split(buffer, data, size, begin);
				process_buffer();
			};

		for(std::filesystem::path file_path : helpers::list_directory_files_recursively(path))
		{	std::ifstream file(file_path.string(), std::ios::binary);

			file_path = std::filesystem::relative(file_path, path);
			logger::message(std::format("Archiving file '{}'...", file_path.string()));

			file.seekg(0, std::ios::end);
			encoded_size_t file_size = file.tellg();
			file.seekg(0, std::ios::beg);

			file_header header = { .size = file_size };
			strcpy(header.path, file_path.string().c_str());
			write_buffer(&header, sizeof(header));

			encoded_size_t left = file_size;
			while(left)
			{	encoded_size_t size = std::min(constants.split_size, left);
				left -= size;
				std::unique_ptr data = std::make_unique<byte[]>(size);
				file.read((char*)data.get(), size);
				write_buffer(data.get(), size);
			}
		}

		process_buffer(true);
	}

	void unarchive(std::filesystem::path path, std::function<std::optional<split_t>()> get_next_split)
	{	logger::message(std::format("Unarchiving into directory '{}'...", path.string()));
		split_t buffer;
		struct file_data
		{	file_header header;
			std::ofstream stream;
			encoded_size_t left;
		};
		std::optional<file_data> file_data;
		for(;;)
		{	auto pop =
				[]<typename type>(split_t & split)
				{	type data = *(type*)split.data();
					split.erase(split.begin(), split.begin() + sizeof(type));
					return data;
				};

			if
			(	buffer.empty() ||
				(	file_data &&
					(buffer.size() < file_data->left)
				)
			)
			{	std::optional<split_t> optional_split = get_next_split();
				if(!optional_split && buffer.empty())
					break;
				split_t split = *optional_split;
				split_header header = pop.operator()<struct split_header>(split);
				if(header.compressed_size < header.uncompressed_size)
					split = compressor::decompress(split, header.uncompressed_size);
				buffer.insert(buffer.end(), split.begin(), split.end());
			}

			if(!file_data)
			{	file_data.emplace();
				file_data->header = pop.operator()<file_header>(buffer);
				logger::message(std::format("Unarchiving file '{}'...", file_data->header.path));
				file_data->left = file_data->header.size;
				std::filesystem::path absolute_path = path / file_data->header.path;
				std::function<void(std::filesystem::path)> create_parent_directory =
					[&create_parent_directory](std::filesystem::path path)
					{	if(path.has_parent_path() && path != path.root_path())
						{	std::filesystem::path parent_path = path.parent_path();
							create_parent_directory(parent_path);
							if(!std::filesystem::exists(parent_path))
								std::filesystem::create_directory(parent_path);
						}
					};
				create_parent_directory(absolute_path);
				file_data->stream = std::ofstream(absolute_path.string(), std::ios::binary);
				file_data->stream.exceptions(std::ios::badbit | std::ios::failbit);
			}

			if(!buffer.empty() && file_data->left)
			{	encoded_size_t size = std::min<encoded_size_t>(file_data->left, buffer.size());
				file_data->left -= size;
				file_data->stream.write((char*)buffer.data(), size);
				buffer.erase(buffer.begin(), buffer.begin() + size);
			}

			if(!file_data->left)
				file_data.reset();
		}
		if(file_data)
			throw
				std::runtime_error
				(	std::format
					(	"Couldn't fully recover file : '{}'.",
						file_data->header.path
					)
				);
	}
}
