#pragma once
#include <ucsl/bitset.h>
#include <ucsl/magic.h>
#include <rip/binary/stream.h>
#include <rip/util/byteswap.h>

namespace rip::binary::containers::needle_archive::v1 {
    struct ChunkHeader {
        ucsl::magic_t<8> magic; // NEDMDLV5 | NEDMATV5 | NEDLDIV1 | NEDTEXV1
        char name[8];
        unsigned int chunkSize;
    };

    struct FileHeader {
        ucsl::magic_t<8> magic; // NEDARCV1
        unsigned int fileSize;
        char name[4]; // usually "arc"
    };
}

namespace rip::util {
    template<> inline void byteswap_deep(rip::binary::containers::needle_archive::v1::ChunkHeader& value) noexcept {
        byteswap_deep(value.chunkSize);
    }

    template<> inline void byteswap_deep(rip::binary::containers::needle_archive::v1::FileHeader& value) noexcept {
        byteswap_deep(value.fileSize);
    }
}

namespace rip::binary::containers::needle_archive::v1 {
    template<typename AddressType, std::endian endianness = std::endian::big>
    class NeedleArchiveReader {
    public:
        struct ChunkReader {
            ChunkHeader& header;
            binary_istream<AddressType> stream;
        };

    private:
        fast_istream raw_stream;
        binary_istream<AddressType> stream;
        FileHeader header;

    public:
        NeedleArchiveReader(std::istream& stream_) : raw_stream{ stream_ }, stream{ raw_stream, endianness } {
            stream.read(header);
        }

        template<typename F>
        void forEachChunk(F f) {
            size_t offset{ stream.tellg() };

            while (offset < header.fileSize + 8) {
                ChunkHeader header;

                stream.read(header);

                auto chunkStart = stream.tellg();
                
                ChunkReader chunkReader{ header, { raw_stream, endianness, chunkStart } };

                f(chunkReader);

                offset = chunkStart + header.chunkSize;

                stream.seekg(offset);
            }
        }
    };

    //template<typename AddressType, std::endian endianness = std::endian::big>
    //class NeedleArchiveWriter {
    //private:
    //    fast_ostream raw_stream;
    //    binary_ostream<AddressType, endianness> stream;
    //    std::vector<unsigned int> addressLocations{};

    //public:
    //    class chunk_ostream : public binary_ostream<AddressType, endianness> {
    //    protected:
    //        NeedleArchiveWriter& writer;
    //        const ucsl::magic_t<8> magic{};
    //        unsigned int version{};
    //        size_t chunkOffset{};
    //        unsigned int flags{};

    //    public:
    //        chunk_ostream(const ucsl::magic_t<8>& magic, unsigned int version, NeedleArchiveWriter& writer, unsigned int flags, bool is_last = false) : magic{ magic }, version{ version }, writer{ writer }, binary_ostream<AddressType, endianness>{ writer.raw_stream, 0x10 }, chunkOffset{ this->tellp() }, flags{ flags | (is_last ? ChunkHeader::LAST_CHILD : 0) } {
    //            this->write(ChunkHeader{});
    //        }

    //        ~chunk_ostream() {
    //            finish();
    //        }

    //        static constexpr bool hasNativeStrings = false;

    //        template<typename T>
    //        void write(const T& obj) {
    //            binary_ostream<AddressType, endianness>::write(obj);
    //        }

    //        template<typename T>
    //        void write(const offset_t<T>& obj) {
    //            if (obj.has_value())
    //                writer.addressLocations.push_back(static_cast<unsigned int>(this->tellp()));

    //            binary_ostream<AddressType, endianness>::write(obj);
    //        }

    //        void finish() {
    //            this->write_padding(4);
    //            size_t chunkEnd = this->tellp();

    //            ChunkHeader chunkHeader{};
    //            chunkHeader.chunkSizeAndFlags = static_cast<unsigned int>(chunkEnd - chunkOffset) | flags;
    //            chunkHeader.version = version;
    //            chunkHeader.magic = magic;

    //            this->seekp(chunkOffset);
    //            this->write(chunkHeader);
    //            this->seekp(chunkEnd);

    //            this->write_padding(16);
    //        }
    //    };

    //    class leaf_chunk_ostream : public chunk_ostream {
    //    public:
    //        leaf_chunk_ostream(const ucsl::magic_t<8>& magic, unsigned int version, NeedleArchiveWriter& writer, unsigned int flags) : chunk_ostream{ magic, version, writer, ChunkHeader::LEAF | flags } {}
    //    };

    //    class branch_chunk_ostream : public chunk_ostream {
    //    public:
    //        branch_chunk_ostream(const ucsl::magic_t<8>& magic, unsigned int version, NeedleArchiveWriter& writer, unsigned int flags) : chunk_ostream{ magic, version, writer, flags } {}

    //        branch_chunk_ostream add_branch_chunk(const ucsl::magic_t<8>& magic, unsigned int version) {
    //            return { magic, version, this->writer, 0 };
    //        }

    //        branch_chunk_ostream add_last_branch_chunk(const ucsl::magic_t<8>& magic, unsigned int version) {
    //            return { magic, version, this->writer, ChunkHeader::LAST_CHILD };
    //        }

    //        leaf_chunk_ostream add_leaf_chunk(const ucsl::magic_t<8>& magic, unsigned int version) {
    //            return { magic, version, this->writer, 0 };
    //        }

    //        leaf_chunk_ostream add_last_leaf_chunk(const ucsl::magic_t<8>& magic, unsigned int version) {
    //            return { magic, version, this->writer, ChunkHeader::LAST_CHILD };
    //        }
    //    };

    //public:
    //    NeedleArchiveWriter(std::ostream& stream_) : raw_stream{ stream_ }, stream{ raw_stream } {
    //        stream.write(FileHeader{});
    //    }

    //    ~NeedleArchiveWriter() {
    //        finish();
    //    }

    //    branch_chunk_ostream add_root_chunk(const ucsl::magic_t<8>& magic, unsigned int version) {
    //        return { magic, version, *this, ChunkHeader::LAST_CHILD };
    //    }

    //    void finish() {
    //        size_t offsetTableOffset = stream.tellp();
    //        writeAddressResolutionChunk();
    //        size_t pos = stream.tellp();

    //        FileHeader fileHeader{};
    //        fileHeader.fileSize = static_cast<unsigned int>(pos) | 0x80000000;
    //        fileHeader.magic = 0x0133054A;
    //        fileHeader.offsetCount = addressLocations.size();
    //        fileHeader.offsetTableOffset = offsetTableOffset;

    //        stream.seekp(0);
    //        stream.write(fileHeader);
    //        stream.seekp(pos);
    //    }

    //    void writeAddressResolutionChunk() {
    //        for (unsigned int addressLocation : addressLocations)
    //            stream.write(addressLocation);
    //    }
    //};
}
