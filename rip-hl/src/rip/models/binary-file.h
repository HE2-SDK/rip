#pragma once
#include "raw.h"

namespace rip::models {
	template<typename T, typename AllocatorSystem>
	struct BinaryFile {
		Raw<T, AllocatorSystem> data{};

		inline BinaryFile() {}
		inline BinaryFile(const BinaryFile<T, AllocatorSystem>& other) = delete;
		inline BinaryFile(Raw<T, AllocatorSystem>&& data) : data{ std::move(data) } {}
		inline BinaryFile(BinaryFile<T, AllocatorSystem>&& other) : data{ std::move(other.data) } {}
	};

	template<typename T, typename AllocatorSystem>
	struct BinaryFileV1 : public BinaryFile<T, AllocatorSystem> {
		using BinaryFileV1<T, AllocatorSystem>::BinaryFileV1;
	};

	template<typename T, typename AllocatorSystem>
	struct BinaryFileV2 : public BinaryFile<T, AllocatorSystem> {
		using BinaryFileV2<T, AllocatorSystem>::BinaryFileV2;
	};
}
