#pragma once
#include <bit>
#include <map>
#include <ucsl/magic.h>
#include <rip/binary/stream.h>
#include <rip/util/byteswap.h>
#include <iostream>
#include <vector>
#include "common.h"

namespace rip::binary::containers::binary_file::v2 {
	struct FileHeader {
		ucsl::magic_t<4> magic;
		ucsl::magic_t<3> version;
		char endianness;
		unsigned int fileSize;
		unsigned short chunkCount;
		unsigned char flags;

		inline void byteswap_deep() noexcept {
			util::byteswap_deep(fileSize);
			util::byteswap_deep(chunkCount);
		}
	};

	struct ChunkHeader {
		ucsl::magic_t<4> magic;
		unsigned int size;
		unsigned int dataSize;
		unsigned int stringTableSize;
		unsigned int offsetTableSize;
		unsigned short additionalHeaderSize;

		inline void byteswap_deep() noexcept {
			util::byteswap_deep(size);
			util::byteswap_deep(dataSize);
			util::byteswap_deep(stringTableSize);
			util::byteswap_deep(offsetTableSize);
			util::byteswap_deep(additionalHeaderSize);
		}
	};

	template<typename RawStreamType, typename AddressType>
	class chunk_istream : public data_istream<RawStreamType, AddressType> {
		ChunkHeader header;

	public:
		chunk_istream(RawStreamType& raw_stream, binary_istream<RawStreamType, AddressType>& stream, std::endian endianness) : data_istream<RawStreamType, AddressType>{ raw_stream, endianness } {
			stream.read(header);
			stream.skip_padding_bytes(header.additionalHeaderSize);
			this->offset_base = stream.tellg();
			
			//this->seekg(header.dataSize);
			//this->readStringTable(header.stringTableSize);
			//this->seekg(0);
		}
	};

	template<typename RawStreamType, typename AddressType, std::endian endianness>
	class chunk_ostream : public data_ostream<RawStreamType, AddressType, endianness> {
		binary_ostream<RawStreamType, AddressType, endianness>& stream;
		const ucsl::magic_t<4> magic{};
		size_t chunkOffset{};
		unsigned short additionalHeaderSize{};

	public:
		chunk_ostream(const ucsl::magic_t<4>& magic, RawStreamType& raw_stream, binary_ostream<RawStreamType, AddressType, endianness>& stream, unsigned short additionalHeaderSize = 0x18) : stream{ stream }, magic { magic }, additionalHeaderSize{ additionalHeaderSize }, chunkOffset{ raw_stream.tellp() }, data_ostream<RawStreamType, AddressType, endianness>{ raw_stream } {
			stream.write(ChunkHeader{});
			stream.write_padding_bytes(additionalHeaderSize);
			this->offset_base = stream.tellp();
		}

		~chunk_ostream() {
			finish();
		}

		void finish() {
			size_t stringTableStart = this->stream.tellp();
			this->writeStringTable();
			size_t offsetTableStart = this->stream.tellp();
			this->writeOffsetTable();
			size_t chunkEnd = this->stream.tellp();

			ChunkHeader chunkHeader{};
			chunkHeader.magic = magic;
			chunkHeader.size = static_cast<unsigned int>(chunkEnd - chunkOffset);
			chunkHeader.dataSize = static_cast<unsigned int>(stringTableStart - sizeof(ChunkHeader) - additionalHeaderSize - chunkOffset);
			chunkHeader.stringTableSize = static_cast<unsigned int>(offsetTableStart - stringTableStart);
			chunkHeader.offsetTableSize = static_cast<unsigned int>(chunkEnd - offsetTableStart);
			chunkHeader.additionalHeaderSize = additionalHeaderSize;

			this->stream.seekp(chunkOffset);
			this->stream.write(chunkHeader);
			this->stream.seekp(chunkEnd);
		}
	};

	template<typename RawStreamType, typename AddrType>
	class BinaryFileReader {
		RawStreamType& raw_stream;
		binary_istream<RawStreamType, AddrType> stream;
		FileHeader header;

	public:
		std::endian endianness;

		BinaryFileReader(RawStreamType& raw_stream) : raw_stream{ raw_stream }, stream { raw_stream } {
			stream.read(header);
			stream.seekg(0);
			stream.endianness = header.endianness == 'B' ? std::endian::big : std::endian::little;
			stream.read(header);
		}

		chunk_istream<RawStreamType, AddrType> getNextDataChunk() {
			return { raw_stream, stream, header.endianness == 'B' ? std::endian::big : std::endian::little };
		}
	};

	template<typename RawStreamType, typename AddrType, std::endian endianness = std::endian::native>
	class BinaryFileWriter {
		RawStreamType& raw_stream;
		binary_ostream<RawStreamType, AddrType, endianness> stream;
		unsigned short chunkCount{};

	public:
		BinaryFileWriter(RawStreamType& stream_) : raw_stream{ stream_ }, stream{ raw_stream } {
			stream.write(FileHeader{});
		}

		~BinaryFileWriter() {
			finish();
		}

		chunk_ostream<RawStreamType, AddrType, endianness> addDataChunk() {
			chunkCount++;
			return { "DATA", raw_stream, stream };
		}

		void finish() {
			size_t pos = stream.tellp();

			FileHeader fileHeader{};
			fileHeader.magic = "BINA";
			fileHeader.version = "210";
			fileHeader.endianness = endianness == std::endian::big ? 'B' : 'L';
			fileHeader.fileSize = static_cast<unsigned int>(pos);
			fileHeader.chunkCount = chunkCount;

			stream.seekp(0);
			stream.write(fileHeader);
			stream.seekp(pos);
		}
	};

	class BinaryFileResolver {
		FileHeader* file;

		template<typename F>
		void forEachChunk(F f) {
			size_t offset{ sizeof(FileHeader) };
			for (unsigned int i = 0; i < file->chunkCount; i++) {
				auto* chunk = reinterpret_cast<ChunkHeader*>(addptr(file, offset));

				f(chunk);

				offset += chunk->size;
			}
		}

		void doByteswaps() {
			util::byteswap_deep(*file);

			forEachChunk([](ChunkHeader* chunk) { util::byteswap_deep(*chunk); });
		}

		void resolveAddresses() {
			forEachChunk([=, this](ChunkHeader* chunk) {
				void* dataStart = addptr(chunk, sizeof(ChunkHeader) + chunk->additionalHeaderSize);
				void* offsetLoc = dataStart;
				void* offsetsStart = addptr(chunk, sizeof(ChunkHeader) + chunk->additionalHeaderSize + chunk->dataSize + chunk->stringTableSize);
				void* offsets = offsetsStart;

				while (offsets != addptr(offsetsStart, chunk->offsetTableSize) && (*static_cast<unsigned char*>(offsets) & 0xC0) != 0) {
					switch ((*static_cast<unsigned char*>(offsets) & 0xC0) >> 6) {
					case 1: offsetLoc = addptr(offsetLoc, (*static_cast<unsigned char*>(offsets) & 0x3F) << 2); offsets = addptr(offsets, 1); break;
					case 2: offsetLoc = addptr(offsetLoc, (util::byteswap_to_native(std::endian::big, *static_cast<unsigned short*>(offsets)) & 0x3FFF) << 2); offsets = addptr(offsets, 2); break;
					case 3: offsetLoc = addptr(offsetLoc, (util::byteswap_to_native(std::endian::big, *static_cast<unsigned int*>(offsets)) & 0x3FFFFFFF) << 2); offsets = addptr(offsets, 4); break;
					}

					size_t* offset = static_cast<size_t*>(offsetLoc);

					util::byteswap_deep_to_native(file->endianness == 'B' ? std::endian::big : std::endian::little, *offset);

					if (*offset != 0)
						*offset += reinterpret_cast<size_t>(dataStart);
				}
			});
		}

	public:
		inline BinaryFileResolver(void* file_) : file{ static_cast<FileHeader*>(file_) } {
			if (file->flags & 1)
				return;

			if (!(file->flags & 2) && std::endian::native != (file->endianness == 'B' ? std::endian::big : std::endian::little))
				doByteswaps();

			resolveAddresses();
		}

		void* getData(unsigned short chunkId) {
			if (chunkId >= file->chunkCount)
				return nullptr;

			size_t offset{ sizeof(FileHeader) };
			for (unsigned short i = 0; i < chunkId; i++)
				offset += reinterpret_cast<ChunkHeader*>(addptr(file, offset))->size;

			auto* chunk = reinterpret_cast<ChunkHeader*>(addptr(file, offset));

			return addptr(chunk, sizeof(ChunkHeader) + chunk->additionalHeaderSize);
		}
	};

	//template<typename AddrType>
	//class BinaryFileDeserializer {
	//	BinaryFileReader<AddrType> container;

	//public:
	//	BinaryFileDeserializer(std::istream& stream) : container{ stream } {}

	//	template<typename GameInterface, typename T, typename R>
	//	T* deserialize(R refl) {
	//		auto chunk = container.getNextDataChunk();

	//		rip::binary::ReflectionDeserializer<GameInterface, decltype(chunk)> deserializer{chunk};

	//		return deserializer.deserialize<T, R>(refl);
	//	}

	//	template<typename GameInterface, typename T>
	//	T* deserialize() {
	//		return deserialize<GameInterface, T>(ucsl::reflection::providers::simplerfl<GameInterface>::template reflect<T>());
	//	}
	//};

	//template<typename AddrType, std::endian endianness = std::endian::native>
	//class BinaryFileSerializer {
	//	BinaryFileWriter<AddrType, endianness> container;

	//public:
	//	BinaryFileSerializer(std::ostream& stream) : container{ stream } {}

	//	template<typename T, typename R>
	//	void serialize(T& data, R refl) {
	//		auto chunk = container.addDataChunk();

	//		rip::binary::ReflectionSerializer serializer{ chunk };

	//		serializer.serialize(data, refl);
	//	}

	//	template<typename GameInterface, typename T>
	//	void serialize(T& data) {
	//		serialize(data, ucsl::reflection::providers::simplerfl<GameInterface>::template reflect<T>());
	//	}
	//};
}
