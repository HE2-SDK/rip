#pragma once
#include <bit>
#include <map>
#include <ucsl/magic.h>
#include <ucsl-reflection/providers/simplerfl.h>
#include <rip/binary/stream.h>
#include <rip/util/byteswap.h>
#include <rip/binary/serialization/ReflectionDeserializer.h>
#include <rip/binary/serialization/ReflectionSerializer.h>
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

	//class chunk_ostream : public offset_binary_ostream {
	//	binary_ostream& stream;
	//	const ucsl::magic_t<4> magic{};
	//	size_t chunkOffset{};
	//	unsigned short additionalHeaderSize{};
	//	std::vector<std::string> strings{}; // This seems superfluous but it is here to keep the discovery order, to generate a file that is closer to official files.
	//	std::map<std::string, std::vector<size_t>> stringOffsets{};
	//	std::vector<size_t> offsets{};

	//	void writeStringTable();
	//	void writeOffsetTable();

	//public:
	//	static constexpr bool hasNativeStrings = true;

	//	chunk_ostream(binary_ostream& stream);
	//	~chunk_ostream();

	//	template<typename T> void write(const T& obj) {
	//		stream.write(obj);
	//	}

	//	template<> void write(const char* const& obj) {
	//		if (obj != nullptr) {
	//			auto i = stringOffsets.find(obj);

	//			if (i == stringOffsets.end()) {
	//				strings.emplace_back(obj);
	//				stringOffsets[obj] = { tellp() };
	//			}
	//			else
	//				i->second.emplace_back(tellp());

	//			offsets.emplace_back(tellp());
	//		}

	//		stream.write(0ull);
	//	}

	//	template<typename T> void write(const serialized_types::o64_t<T>& obj) {
	//		offsets.emplace_back(tellp());
	//		stream.write(obj.has_value() ? obj.value() : 0ull);
	//	}

	//	template<typename T> void write(const serialized_types::o32_t<T>& obj) {
	//		offsets.emplace_back(tellp());
	//		stream.write(obj.has_value() ? obj.value() : 0u);
	//	}

	//	void finish();
	//};

	template<typename RawStreamType, typename AddrType>
	class BinaryFileReader {
		RawStreamType& raw_stream;
		binary_istream<RawStreamType, AddrType> stream;
		FileHeader header;

	public:
		inline BinaryFileReader(RawStreamType& raw_stream) : raw_stream{ raw_stream }, stream{ raw_stream } {
			stream.read(header);
			stream.seekg(0);
			stream.endianness = header.endianness == 'B' ? std::endian::big : std::endian::little;
			stream.read(header);
		}

		inline data_istream<RawStreamType, AddrType> getData() {
			return { raw_stream, stream, header.endianness == 'B' ? std::endian::big : std::endian::little };
		}
	};

	//template<typename RawStreamType, typename AddrType, std::endian endianness>
	//class BinaryFileWriter {
	//	RawStreamType& raw_stream;
	//	binary_ostream<RawStreamType, AddrType, endianness> stream;

	//public:
	//	inline BinaryFileWriter(RawStreamType& stream) : raw_stream{ raw_stream }, stream{ raw_stream } {
	//		stream.write(FileHeader{});
	//	}

	//	inline ~BinaryFileWriter() {
	//		finish();
	//	}

	//	inline data_ostream< addData() {

	//	}
	//	inline void finish();
	//};
}
