#pragma once
#include "common.h"

namespace rip::models {
	template<typename T, typename AllocatorSystem, bool include_bvh_ = false>
	struct BinaryFileV1 : public BinaryFile<T, AllocatorSystem> {
		using BinaryFile<T, AllocatorSystem>::BinaryFile;

		static constexpr bool include_bvh = include_bvh_;
	};
}
