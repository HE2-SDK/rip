#pragma once
#include <rip/models/raw.h>

namespace rip::models {
	template<typename T, typename AllocatorSystem>
	struct BinaryFile {
		Raw<T, AllocatorSystem> data{};

		inline BinaryFile() {}
		inline BinaryFile(const BinaryFile<T, AllocatorSystem>& other) = delete;
		inline BinaryFile(Raw<T, AllocatorSystem>&& data) : data{ std::move(data) } {}
		inline BinaryFile(BinaryFile<T, AllocatorSystem>&& other) : data{ std::move(other.data) } {}
	};
}
