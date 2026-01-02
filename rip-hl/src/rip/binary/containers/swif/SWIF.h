#pragma once
#include <vector>
#include <simple-reflection/simple-reflection.h>
#include <ucsl-reflection/providers/simplerfl.h>
#include <rip/binary/serialization/ReflectionSerializer.h>
#include <rip/binary/stream.h>
#include <rip/util/byteswap.h>

namespace rip::binary::containers::swif::v1 {
	static thread_local unsigned int gTextureListCount;
	template<typename T>
	struct TextureListArray {};

	inline size_t GetTextureListCount(const int& parent) { return gTextureListCount; }
	template<typename T> using TextureListArrayRefl = ::simplerfl::dynamic_carray<T, ::simplerfl::selector_resolver<size_t, int, GetTextureListCount>>;

    struct SRS_CHUNK_HEADER {
        unsigned int magic;
        unsigned int chunkSize;
    };

    struct SRS_BINARY_FILE_HEADER_CHUNK_HEADER {
        unsigned int chunkCount;
        unsigned int chunksStart;
        unsigned int chunksSize;
        unsigned int addressResolutionHeaderOffset;
        unsigned int revision;
    };

    struct SRS_TEXTURELIST_CHUNK_HEADER {
        unsigned int startOffset;
        unsigned int textureListCount;
    };

    struct SRS_PROJECT_CHUNK_HEADER {
        unsigned int startOffset;
    };

    struct SRS_ADDRESS_RESOLUTION_CHUNK_HEADER {
        unsigned int addressToResolveCount;
        unsigned int isResolved; // 0 if not, 1 if yes
    };
}

namespace simplerfl {
	template<typename T> struct canonical<rip::binary::containers::swif::v1::TextureListArray<T>> { using type = rip::binary::containers::swif::v1::TextureListArrayRefl<T>; };
}

namespace rip::util {
	template<> inline void byteswap_deep(rip::binary::containers::swif::v1::SRS_BINARY_FILE_HEADER_CHUNK_HEADER& value) noexcept {
		byteswap_deep(value.chunkCount);
		byteswap_deep(value.chunksStart);
		byteswap_deep(value.chunksSize);
		byteswap_deep(value.addressResolutionHeaderOffset);
		byteswap_deep(value.revision);
		byteswap_deep(value.addressResolutionHeaderOffset);
	}

	template<> inline void byteswap_deep(rip::binary::containers::swif::v1::SRS_ADDRESS_RESOLUTION_CHUNK_HEADER& value) noexcept {
		byteswap_deep(value.addressToResolveCount);
		byteswap_deep(value.isResolved);
	}

	template<> inline void byteswap_deep(rip::binary::containers::swif::v1::SRS_CHUNK_HEADER& value) noexcept {
		byteswap_deep(value.magic);
		byteswap_deep(value.chunkSize);
	}

	template<> inline void byteswap_deep(rip::binary::containers::swif::v1::SRS_TEXTURELIST_CHUNK_HEADER& value) noexcept {
		byteswap_deep(value.startOffset);
		byteswap_deep(value.textureListCount);
	}

	template<> inline void byteswap_deep(rip::binary::containers::swif::v1::SRS_PROJECT_CHUNK_HEADER& value) noexcept {
		byteswap_deep(value.startOffset);
	}
}

namespace rip::binary::containers::swif::v1 {
	static constexpr const unsigned int SWIF = 0x46495753;
	static constexpr const unsigned int SWTL = 0x4C545753;
	static constexpr const unsigned int SWPR = 0x52505753;
	static constexpr const unsigned int SEND = 0x444E4553;
	static constexpr const unsigned int SOF0 = 0x30464F53;

	class SWIFSerializer {
		class swif_ostream : public binary_ostream<size_t> {
			SWIFSerializer& serializer;

		public:
			swif_ostream(SWIFSerializer& serializer);

			static constexpr bool hasNativeStrings = false;

			template<typename T>
			void write(const T& obj) {
				binary_ostream<size_t>::write(obj);
			}

			template<typename T>
			void write(const offset_t<T>& obj) {
				if (obj.has_value())
					serializer.addressLocations.push_back(static_cast<unsigned int>(tellp()));
				
				binary_ostream<size_t>::write(obj);
			}
		};

		fast_ostream rawStream;
		swif_ostream stream{ *this };
		ReflectionSerializer<swif_ostream> reflectionSerializer{ stream };
		std::vector<unsigned int> addressLocations{};
		size_t addressResolutionChunkOffset{};
		size_t chunksStart{};
		unsigned int chunkCount{};

		template<typename F>
		void writeChunk(unsigned int magic, F&& writeFunc) {
			size_t start = stream.tellp();

			rip::binary::containers::swif::v1::SRS_CHUNK_HEADER chunkHeader{};
			chunkHeader.magic = magic;
			chunkHeader.chunkSize = 0;

			stream.write(chunkHeader);

			size_t dataStart = stream.tellp();

			writeFunc();
			stream.write_padding(16);

			size_t end = stream.tellp();
			chunkHeader.chunkSize = static_cast<unsigned int>(end - dataStart);

			stream.seekp(start);
			stream.write(chunkHeader);
			stream.seekp(end);
		}

		template<typename F>
		void writeMainChunk(unsigned int magic, F&& writeFunc) {
			writeChunk(magic, writeFunc);
			chunkCount++;
		}

		void writeBinaryFileHeaderChunk();

		template<typename GameInterface, typename T>
		void writeTextureListChunk(T* textureLists, size_t textureListCount) {
			writeMainChunk(SWTL, [&]() {
				TextureListArray<T>& textureListArr{ *reinterpret_cast<TextureListArray<T>*>(textureLists) };

				rip::binary::containers::swif::v1::SRS_TEXTURELIST_CHUNK_HEADER header{};
				header.startOffset = 16;
				header.textureListCount = static_cast<unsigned int>(textureListCount);

				stream.write(header);
				stream.write_padding(16);
				gTextureListCount = static_cast<unsigned int>(textureListCount);
				reflectionSerializer.serialize(textureListArr, ucsl::reflection::providers::simplerfl<GameInterface>::reflect(textureListArr));
			});
		}

		template<typename GameInterface, typename P>
		void writeProjectChunk(P& project) {
			writeMainChunk(SWPR, [&]() {
				rip::binary::containers::swif::v1::SRS_PROJECT_CHUNK_HEADER header{};
				header.startOffset = 16;

				stream.write(header);
				stream.write_padding(16);
				reflectionSerializer.serialize(project, ucsl::reflection::providers::simplerfl<GameInterface>::reflect(project));
			});
		}

		void writeAddressResolutionChunk();
		void writeEndChunk();
	public:
		SWIFSerializer(std::ostream& stream);
		~SWIFSerializer();

		template<typename GameInterface, typename P>
		void serialize(P& project) {
			writeTextureListChunk<GameInterface>(project.textureLists, project.textureListCount);
			writeProjectChunk<GameInterface>(project);
		}
	};

	class SWIFResolver {
		rip::binary::containers::swif::v1::SRS_CHUNK_HEADER* file;

		void resolveAddresses();

		template<typename F>
		inline void forEachChunk(F f) {
			// Note: SRS_BINARY_FILE_HEADER_CHUNK_HEADER is always the first chunk.
			rip::binary::containers::swif::v1::SRS_BINARY_FILE_HEADER_CHUNK_HEADER* binaryFileHeader = (rip::binary::containers::swif::v1::SRS_BINARY_FILE_HEADER_CHUNK_HEADER*)&file[1];

			auto* chunk = addptr(&file[1], file->chunkSize);

			for (unsigned int i = 0; i < binaryFileHeader->chunkCount; i++, chunk = addptr(&chunk[1], chunk->chunkSize))
				f(chunk);
		}

	public:
		SWIFResolver(void* file);

		template<typename P>
		inline P* getProject() {
			P* result{};

			forEachChunk([&result](rip::binary::containers::swif::v1::SRS_CHUNK_HEADER* chunk) {
				if (chunk->magic == SWPR) {
					auto projHeader = (rip::binary::containers::swif::v1::SRS_PROJECT_CHUNK_HEADER*)&chunk[1];

					result = (P*)addptr(chunk, projHeader->startOffset);
				}
			});

			return result;
		}
	};
}
