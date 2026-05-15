#pragma once
#include <vector>
#include <map>
#include <rip/binary/stream.h>
#include <rip/binary/types.h>
#include <rip/util/byteswap.h>

namespace rip::binary::containers::binary_file {
	template<typename RawStreamType, typename AddressType>
	class data_istream : public binary_istream<RawStreamType, AddressType> {
	protected:
		std::map<size_t, std::string> strings{};

		void readStringTable(size_t tableSize) {
			auto startPos = this->tellg();
			auto pos = startPos;
			std::string str{};

			while (pos < startPos + tableSize) {
				this->read_string(str);
				strings[pos] = std::move(str);
				pos = this->tellg();
			}
		}

	public:
		static constexpr bool hasNativeStrings = true;

		data_istream(RawStreamType& raw_stream, std::endian endianness) : binary_istream<RawStreamType, AddressType>{ raw_stream, endianness, raw_stream.tellg() } {}

		template<typename T> void read(T& obj) {
			binary_istream<RawStreamType, AddressType>::read(obj);
		}

		template<> void read(const char*& obj) {
			AddressType stroff;
			binary_istream<RawStreamType, AddressType>::read(stroff);

			if (stroff == 0)
				obj = nullptr;
			else if (strings.contains(stroff))
				obj = strings[static_cast<size_t>(stroff)].c_str();
			else {
				auto pos = this->tellg();
				this->seekg(stroff);
				std::string str{};
				this->read_string(str);
				strings[static_cast<size_t>(stroff)] = std::move(str);
				this->seekg(pos);
			}
		}
	};

	template<typename RawStreamType, typename AddressType, std::endian endianness>
	class data_ostream : public binary_ostream<RawStreamType, AddressType, endianness> {
	protected:
		std::vector<std::string> strings{}; // This seems superfluous but it is here to keep the discovery order, to generate a file that is closer to official files.
		std::map<std::string, std::vector<size_t>> stringOffsets{};
		std::vector<size_t> offsets{};

		void writeStringTable() {
			for (auto& string : strings) {
				auto offsets = stringOffsets[string];
				auto pos = this->tellp();

				for (auto offset : offsets) {
					this->seekp(offset);
					write(static_cast<AddressType>(pos));
				}

				this->seekp(pos);
				this->write_string(string.c_str());
			}

			this->write_padding(4);
		}

		void writeOffsetTable() {
			size_t last_offset = 0;

			for (size_t offset : offsets) {
				size_t diff = offset - last_offset;

				if (diff >= (1 << 16))
					this->template write<unsigned int>(util::byteswap_to_native(std::endian::big, static_cast<unsigned int>((diff >> 2u) | (3u << 30u))));
				else if (diff >= (1 << 8))
					this->template write<unsigned short>(util::byteswap_to_native(std::endian::big, static_cast<unsigned short>((diff >> 2u) | (2u << 14u))));
				else
					this->template write<unsigned char>(static_cast<unsigned char>((diff >> 2u) | (1u << 6u)));

				last_offset = offset;
			}

			this->write_padding(4);
		}

	public:
		static constexpr bool hasNativeStrings = true;

		data_ostream(RawStreamType& raw_stream) : binary_ostream<RawStreamType, AddressType, endianness>{ raw_stream, raw_stream.tellp() } {}

		template<typename T> void write(const T& obj) {
			binary_ostream<RawStreamType, AddressType, endianness>::write(obj);
		}

		template<typename T> void write(const offset_t<T>& obj) {
			// TODO: remove after impl of read
			//if (obj.has_value())
			offsets.emplace_back(this->tellp());

			binary_ostream<RawStreamType, AddressType, endianness>::write(obj);
		}

		template<> void write(const char* const& obj) {
			if (obj != nullptr) {
				auto i = stringOffsets.find(obj);

				if (i == stringOffsets.end()) {
					strings.emplace_back(obj);
					stringOffsets[obj] = { this->tellp() };
				}
				else
					i->second.emplace_back(this->tellp());

				offsets.emplace_back(this->tellp());
			}

			binary_ostream<RawStreamType, AddressType, endianness>::write(static_cast<AddressType>(0));
		}
	};
}