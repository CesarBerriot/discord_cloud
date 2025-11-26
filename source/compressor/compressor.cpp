#include "compressor.hpp"
#include <format>
#include <lzav.h>
#include "logger/logger.hpp"
#include "helpers/helpers.hpp"

namespace compressor
{	blob_t compress(blob_t blob)
	{	logger::message(std::format("Compressing {} bytes...", blob.size()));
		int max_compressed_size = lzav_compress_bound(blob.size());
		std::unique_ptr buffer = std::make_unique<byte[]>(max_compressed_size);
		int compressed_size =
			lzav_compress_default
			(	blob.data(),
				buffer.get(),
				blob.size(),
				max_compressed_size
			);
		if(!compressed_size)
			throw std::runtime_error("Compression failure.");
		logger::message
		(	std::format
			(	"Compressed to {} bytes ({}%)...",
				compressed_size,
				helpers::compute_percentage(blob.size(), compressed_size)
			)
		);
		blob.assign(buffer.get(), buffer.get() + compressed_size);
		return blob;
	}

	blob_t decompress(blob_t blob, int uncompressed_size)
	{	logger::message
		(	std::format
			(	"Decompressing {} bytes to {} bytes ({}%)...",
				blob.size(),
				uncompressed_size,
				helpers::compute_percentage(blob.size(), uncompressed_size)
			)
		);
		std::unique_ptr buffer = std::make_unique<byte[]>(uncompressed_size);
		int decompression_result =
			lzav_decompress
			(	blob.data(),
				buffer.get(),
				blob.size(),
				uncompressed_size
			);
		if(decompression_result < 0)
			throw std::runtime_error("Decompression failure.");
		logger::message("Decompression done.");
		blob.assign(buffer.get(), buffer.get() + uncompressed_size);
		return blob;
	}
}
