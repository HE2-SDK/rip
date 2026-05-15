#pragma once
#include <bit>

namespace rip::models {
	template<typename R, typename AllocatorSystem, typename AddrType = size_t, std::endian endianness = std::endian::native, bool byteswap_offsets = true, bool relative_offsets = false>
	struct Raw {
		void* data{};

		inline Raw() : data{} {}
		inline Raw(void* data) : data{ data } {}
		inline Raw(const Raw<R, AllocatorSystem, AddrType, endianness, byteswap_offsets, relative_offsets>& other) = delete;
		inline Raw(Raw<R, AllocatorSystem, AddrType, endianness, byteswap_offsets, relative_offsets>&& other) : data{ other.data } {
			other.data = nullptr;
		}
		inline ~Raw() {
			if (data != nullptr)
				AllocatorSystem::get_allocator()->Free(data);
		}
	};
}
