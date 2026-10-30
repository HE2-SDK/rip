#pragma once
#include <bit>
#include <map>
#include <ucsl/magic.h>
#include <rip/binary/stream.h>
#include <rip/util/byteswap.h>
#include <iostream>
#include <vector>
#include "common.h"

namespace rip::binary::containers::binary_file::v1 {
	struct FileHeader {
		unsigned int size;
		unsigned int dataSize;
		unsigned int offsetTableSize;
		unsigned int padding1;
		unsigned short flags;
		unsigned short footerCount;
		ucsl::magic_t<3> version;
		char endianness;
		ucsl::magic_t<4> magic;
		unsigned int padding2;

		inline void byteswap_deep() noexcept {
			util::byteswap_deep(size);
			util::byteswap_deep(dataSize);
			util::byteswap_deep(offsetTableSize);
			util::byteswap_deep(footerCount);
		}
	};

	struct BVHHeader {
		unsigned int unk1;
		unsigned int unk2;
		ucsl::magic_t<3> magic;

		inline void byteswap_deep() noexcept {
			util::byteswap_deep(unk1);
			util::byteswap_deep(unk2);
		}
	};

	template<typename BinaryOutputStreamType, bool include_bvh = false>
	class chunk_ostream : public data_ostream<BinaryOutputStreamType> {
		BinaryOutputStreamType& stream;

	public:
		chunk_ostream(BinaryOutputStreamType& stream) : stream{ stream }, data_ostream<BinaryOutputStreamType>{ stream } {
			stream.write(FileHeader{});
			this->offset_base = stream.tellp();
		}

		~chunk_ostream() {
			finish();
		}

		void finish() {
			this->writeStringTable();
			size_t offsetTableStart = this->stream.tellp();
			this->writeOffsetTable();
			size_t offsetTableEnd = this->stream.tellp();

			unsigned int footerCount{};
			if constexpr (include_bvh) {
				BVHHeader bvhHeader{};
				bvhHeader.unk1 = 0x10;
				bvhHeader.unk2 = 0x0;
				bvhHeader.magic = "bvh";

				this->stream.write(bvhHeader);
				this->stream.write_padding(4);

				footerCount++;
			}

			size_t chunkEnd = this->stream.tellp();

			FileHeader fileHeader{};
			fileHeader.size = static_cast<unsigned int>(chunkEnd);
			fileHeader.dataSize = static_cast<unsigned int>(offsetTableStart - sizeof(FileHeader));
			fileHeader.offsetTableSize = static_cast<unsigned int>(offsetTableEnd - offsetTableStart);
			fileHeader.footerCount = footerCount;
			fileHeader.version = "\000\0001";
			fileHeader.endianness = BinaryOutputStreamType::endianness == std::endian::big ? 'B' : 'L';
			fileHeader.magic = "BINA";

			this->stream.seekp(0);
			this->stream.write(fileHeader);
			this->stream.seekp(chunkEnd);
		}
	};

	template<typename BinaryInputStreamType>
	class BinaryFileReader {
		BinaryInputStreamType stream;
		FileHeader header;

	public:
		inline BinaryFileReader(BinaryInputStreamType& parent_stream) : stream{ parent_stream.get_raw_stream() } {
			stream.read(header);
			stream.seekg(0);
			stream.endianness = header.endianness == 'B' ? std::endian::big : std::endian::little;
			stream.read(header);
		}

		inline data_istream<BinaryInputStreamType> getData() {
			return { stream, header.endianness == 'B' ? std::endian::big : std::endian::little };
		}
	};

	template<typename BinaryOutputStreamType, bool include_bvh = false>
	class BinaryFileWriter {
		BinaryOutputStreamType stream;

	public:
		BinaryFileWriter(BinaryOutputStreamType& parent_stream) : stream{ parent_stream.get_raw_stream() } {}

		chunk_ostream<BinaryOutputStreamType, include_bvh> getDataChunk() {
			return { stream };
		}
	};
}
