#pragma once
#include <ucsl/bitset.h>
#include <ucsl/magic.h>
#include <rip/binary/stream.h>
#include <rip/util/byteswap.h>

namespace rip::binary::containers::mirage::v2 {
    struct NodeHeader {
        enum Flag : unsigned int {
            LEAF = 0x20000000,
            LAST_CHILD = 0x40000000,
        };

        unsigned int nodeSizeAndFlags;
        unsigned int value;
		ucsl::magic_t<8> magic;
    };

    /*
     * File structure is as follows:
     * ---
     * FileHeader
     * ---
     * NodeHeader <- base address of offsets in offset table
     * node data (can be more nodes if it is not a leaf)
     * ---
     * offset table
     */
    struct FileHeader {
        unsigned int fileSize; // MSB is set for a V2 mirage container
        unsigned int magic;
        uint32_t offsetTableOffset;
        uint32_t offsetCount;
    };
}

namespace rip::util {
    template<> inline void byteswap_deep(rip::binary::containers::mirage::v2::NodeHeader& value) noexcept {
        byteswap_deep(value.nodeSizeAndFlags);
        byteswap_deep(value.value);
    }

    template<> inline void byteswap_deep(rip::binary::containers::mirage::v2::FileHeader& value) noexcept {
        byteswap_deep(value.fileSize);
        byteswap_deep(value.magic);
        byteswap_deep(value.offsetTableOffset);
        byteswap_deep(value.offsetCount);
    }
}

namespace rip::binary::containers::mirage::v2 {
	template<typename BinaryInputStreamType>
    class MirageResourceImageReader {
    private:
        BinaryInputStreamType stream;
        FileHeader header;
        std::endian endianness{ std::endian::big };
        size_t offsetBase;

    public:
        class NodeReader {
            MirageResourceImageReader& imgReader;

        public:
            NodeHeader header;

            NodeReader(MirageResourceImageReader& imgReader) : imgReader{ imgReader } {
                imgReader.stream.read(header);
            }

            unsigned int get_size() const {
                return header.nodeSizeAndFlags & 0x1FFFFFFF;
            }

            bool is_leaf() const {
                return header.nodeSizeAndFlags & NodeHeader::LEAF;
            }

            bool is_last_child() const {
                return header.nodeSizeAndFlags & NodeHeader::LAST_CHILD;
            }

            BinaryInputStreamType& get_stream() {
                return imgReader.stream;
            }

            template<typename F>
            void for_each_child(F f) {
                assert(!is_leaf() && "not a branch node");

                bool isLast{};
                do {
                    auto nodeOffset = imgReader.stream.tellg();

                    NodeReader child{ imgReader };

                    f(child);

                    imgReader.stream.seekg(nodeOffset + child.get_size());
                    imgReader.stream.skip_padding(16);
                    isLast = child.is_last_child();
                } while (!isLast);
            }
        };

        MirageResourceImageReader(BinaryInputStreamType& parent_stream) : stream{ parent_stream.get_raw_stream(), parent_stream.endianness, parent_stream.get_raw_stream().tellg() + sizeof(FileHeader) } {
            stream.read(header);
        }

        BinaryInputStreamType& get_stream() {
            return stream;
        }

        NodeReader get_root_node() {
            return { *this };
        }
    };

    template<typename BinaryOutputStreamType>
    class MirageResourceImageWriter {
    private:
        BinaryOutputStreamType stream;
        std::vector<unsigned int> addressLocations{};

    public:
        class node_ostream : public BinaryOutputStreamType {
        protected:
            MirageResourceImageWriter& writer;
            const ucsl::magic_t<8> magic{};
            unsigned int value{};
            size_t nodeOffset{};
            unsigned int flags{};

        public:
            node_ostream(const ucsl::magic_t<8>& magic, unsigned int value, MirageResourceImageWriter& writer, unsigned int flags, bool is_last = false) : magic{ magic }, value{ value }, writer{ writer }, BinaryOutputStreamType{ writer.stream.get_raw_stream(), sizeof(FileHeader) }, nodeOffset{this->tellp()}, flags{flags | (is_last ? NodeHeader::LAST_CHILD : 0)} {
                this->write(NodeHeader{});
            }

            ~node_ostream() {
                finish();
            }

            static constexpr bool hasNativeStrings = false;

            template<typename T>
            void write(const T& obj) {
                BinaryOutputStreamType::write(obj);
            }

            template<typename T>
            void write(const offset_t<T>& obj) {
                if (obj.has_value())
                    writer.addressLocations.push_back(static_cast<unsigned int>(this->tellp()));

                BinaryOutputStreamType::write(obj);
            }

            void finish() {
                this->write_padding(4);
                size_t nodeEnd = this->tellp();

                NodeHeader nodeHeader{};
                nodeHeader.nodeSizeAndFlags = static_cast<unsigned int>(nodeEnd - nodeOffset) | flags;
                nodeHeader.value = value;
                nodeHeader.magic = magic;

                this->seekp(nodeOffset);
                this->write(nodeHeader);
                this->seekp(nodeEnd);

                this->write_padding(16);
            }
        };

        class leaf_node_ostream : public node_ostream {
        public:
            leaf_node_ostream(const ucsl::magic_t<8>& magic, unsigned int value, MirageResourceImageWriter& writer, unsigned int flags) : node_ostream{ magic, value, writer, NodeHeader::LEAF | flags } {}
        };

        class branch_node_ostream : public node_ostream {
        public:
            branch_node_ostream(const ucsl::magic_t<8>& magic, unsigned int value, MirageResourceImageWriter& writer, unsigned int flags) : node_ostream{ magic, value, writer, flags } {}

            branch_node_ostream add_branch_node(const ucsl::magic_t<8>& magic, unsigned int value, bool is_last) {
                return { magic, value, this->writer, is_last ? NodeHeader::LAST_CHILD : 0 };
            }

            leaf_node_ostream add_leaf_node(const ucsl::magic_t<8>& magic, unsigned int value, bool is_last) {
                return { magic, value, this->writer, is_last ? NodeHeader::LAST_CHILD : 0 };
            }
        };

    public:
        MirageResourceImageWriter(BinaryOutputStreamType& parent_stream) : stream{ parent_stream.get_raw_stream(), parent_stream.get_raw_stream().tellp() } { // I added the offset here. If conversion errors, this is probably the cause.
            stream.write(FileHeader{});
        }

        ~MirageResourceImageWriter() {
            finish();
        }

        branch_node_ostream add_root_node(const ucsl::magic_t<8>& magic, unsigned int value) {
            return { magic, value, *this, NodeHeader::LAST_CHILD };
        }

        void finish() {
            size_t offsetTableOffset = stream.tellp();
            writeAddressResolutionChunk();
            size_t pos = stream.tellp();

            FileHeader fileHeader{};
            fileHeader.fileSize = static_cast<unsigned int>(pos) | 0x80000000;
            fileHeader.magic = 0x0133054A;
            fileHeader.offsetCount = addressLocations.size();
            fileHeader.offsetTableOffset = offsetTableOffset;

            stream.seekp(0);
            stream.write(fileHeader);
            stream.seekp(pos);
        }

        void writeAddressResolutionChunk() {
            for (unsigned int addressLocation : addressLocations)
                stream.write(addressLocation);
        }
    };
}
