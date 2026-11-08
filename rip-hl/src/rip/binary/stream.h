#pragma once
#include <iostream>
#include <bit>
#include <ranges>
#include <string>
#include <rip/util/memory.h>
#include <rip/util/byteswap.h>
#include "types.h"

namespace rip::binary {
	namespace internal {
		inline char zeroes[8192]{};
	}

	class fast_istream {
		std::istream& stream;
		size_t shadow_pos; // prevent doing slow tellg() calls.

	public:
		fast_istream(std::istream& stream) : stream{ stream }, shadow_pos{ (size_t)stream.tellg() } {}

		void read(char* str, size_t count) {
			stream.read(str, count);
			shadow_pos += count;
		}

		void read_string(std::string& str) {
			std::getline(stream, str, '\0');
			shadow_pos += str.size() + 1;
		}

		void seekg(size_t loc) {
			if (shadow_pos == loc)
				return;

			stream.seekg(loc);
			shadow_pos = loc;
		}

		size_t tellg() const {
			return shadow_pos;
		}
	};

	class fast_ostream {
		std::ostream& stream;
		size_t shadow_pos; // prevent doing slow tellg() calls.

	public:
		fast_ostream(std::ostream& stream) : stream{ stream }, shadow_pos{ (size_t)stream.tellp() } {}

		void write(const char* str, size_t count) {
			stream.write(str, count);
			shadow_pos += count;
		}

		void write_string(const char* str) {
			size_t size = strlen(str) + 1;
			stream.write(str, size);
			shadow_pos += size;
		}

		void seekp(size_t loc) {
			if (shadow_pos == loc)
				return;

			stream.seekp(loc);
			shadow_pos = loc;
		}

		size_t tellp() const {
			return shadow_pos;
		}
	};

	class mem_istream {
		char* ptr;
		size_t pos;

	public:
		mem_istream(void* ptr) : ptr{ (char*)ptr }, pos{ 0 } {}

		void read(char* str, size_t count) {
			memcpy(str, addptr(ptr, pos), count);
			pos += count;
		}

		void read_string(std::string& str) {
			str = addptr(ptr, pos);
			pos += str.size() + 1;
		}

		void seekg(size_t loc) {
			pos = loc;
		}

		size_t tellg() const {
			return pos;
		}
	};

	class mem_ostream {
		char* ptr;
		size_t pos;

	public:
		mem_ostream(void* ptr) : ptr{ (char*)ptr }, pos{ 0 } {}

		void write(const char* str, size_t count) {
			memcpy(addptr(ptr, pos), str, count);
			pos += count;
		}

		void write_string(const char* str) {
			strcpy(addptr(ptr, pos), str);
			pos += strlen(str) + 1;
		}

		void seekp(size_t loc) {
			pos = loc;
		}

		size_t tellp() const {
			return pos;
		}
	};

	class null_ostream {
		size_t pos;

	public:
		null_ostream() : pos{ 0 } {}

		void write(const char* str, size_t count) {
			pos += count;
		}

		void write_string(const char* str) {
			pos += strlen(str) + 1;
		}

		void seekp(size_t loc) {
			pos = loc;
		}

		size_t tellp() const {
			return pos;
		}
	};

	template<typename RawStreamType_, typename AddrType_, bool byteswap_offsets_ = true, bool relative_offsets_ = false>
	class binary_istream {
	protected:
		RawStreamType_& stream;
		size_t offset_base;

	public:
		typedef RawStreamType_ RawStreamType;
		typedef AddrType_ AddrType;
		std::endian endianness;
		static constexpr bool byteswap_offsets = byteswap_offsets_;
		static constexpr bool relative_offsets = relative_offsets_;

		binary_istream(RawStreamType_& stream, std::endian endianness = std::endian::native, size_t offset_base = 0) : stream{ stream }, endianness{ endianness }, offset_base{ offset_base } {}

		template<typename T, bool byteswap = true>
		void read(T& obj) {
			stream.read(reinterpret_cast<char*>(&obj), sizeof(T));
			
			if constexpr (byteswap)
				util::byteswap_deep_to_native(endianness, obj);
		}

		template<typename T>
		void read(offset_t<T>& obj) {
			size_t offset;
			read_as<AddrType, byteswap_offsets>(offset);

			if (offset == 0)
				obj = offset_t<T>{};
			else {
				if constexpr (relative_offsets)
					offset += this->tellg() - sizeof(AddrType);

				obj = offset_t<T>{ offset };
			}
		}

		void read(size_val_t& obj) {
			read_as<AddrType>(obj);
		}

		void read(ptrdiff_val_t& obj) {
			read_as<AddrType>(obj);
		}

		template<typename U, bool byteswap = true, typename T = U>
		void read_as(T& obj) {
			U value; 
			read<U, byteswap>(value);
			obj = static_cast<T>(value);
		}

		void read_string(std::string& str) {
			stream.read_string(str);
		}

		void skip_padding(size_t alignment) {
			skip_padding_bytes(align(tellg(), alignment) - tellg());
		}

		void skip_padding_bytes(size_t size) {
			seekg(tellg() + size);
		}

		void seekg(size_t loc) {
			stream.seekg(loc + offset_base);
		}

		size_t tellg() const {
			return stream.tellg() - offset_base;
		}

		RawStreamType_& get_raw_stream() {
			return stream;
		}
	};

	template<typename RawStreamType_, typename AddrType_, std::endian endianness_ = std::endian::native, bool byteswap_offsets_ = true, bool relative_offsets_ = false>
	class binary_ostream {
	protected:
		RawStreamType_& stream;
		size_t offset_base;

	public:
		typedef RawStreamType_ RawStreamType;
		typedef AddrType_ AddrType;
		static constexpr bool hasNativeStrings = false;
		static constexpr std::endian endianness = endianness_;
		static constexpr bool byteswap_offsets = byteswap_offsets_;
		static constexpr bool relative_offsets = relative_offsets_;

		binary_ostream(RawStreamType_& stream, size_t offset_base = 0) : stream{ stream }, offset_base{ offset_base } {}

		template<typename T, bool byteswap = true>
		void write(const T& obj) {
			if constexpr (byteswap) {
				T val = obj;
				util::byteswap_deep_to_native(endianness, val);
				stream.write(reinterpret_cast<const char*>(&val), sizeof(T));
			}
			else
				stream.write(reinterpret_cast<const char*>(&obj), sizeof(T));
		}

		//template<typename T, bool byteswap = true>
		//void write(T&& obj) {
		//	if constexpr (byteswap)
		//		util::byteswap_deep_to_native(endianness, obj);
		//	stream.write(reinterpret_cast<const char*>(&obj), sizeof(T));
		//}

		template<typename T>
		void write(const offset_t<T>& obj) {
			if (!obj.has_value())
				write_as<AddrType, byteswap_offsets>(0);
			else {
				size_t off = obj.value();

				if constexpr (relative_offsets)
					off -= this->tellp() - sizeof(AddrType);

				write_as<AddrType, byteswap_offsets>(off);
			}
		}

		void write(const size_val_t& obj) {
			write_as<AddrType>(obj);
		}

		void write(const ptrdiff_val_t& obj) {
			write_as<AddrType>(obj);
		}

		template<typename U, bool byteswap = true, typename T = U>
		void write_as(const T& obj) {
			write<U, byteswap>(static_cast<U>(obj));
		}

		void write_string(const char* str) {
			stream.write_string(str);
		}

		void write_padding(size_t alignment) {
			write_padding_bytes(align(tellp(), alignment) - tellp());
		}

		void write_padding_bytes(size_t size) {
			stream.write(internal::zeroes, size);
		}

		void seekp(size_t loc) {
			stream.seekp(loc + offset_base);
		}

		size_t tellp() const {
			return stream.tellp() - offset_base;
		}

		RawStreamType_& get_raw_stream() {
			return stream;
		}
	};
}
