#pragma once
#include "common.h"

namespace rip::models {
	template<typename T, typename AllocatorSystem>
	struct BinaryFileV2 : public BinaryFile<T, AllocatorSystem> {
		using BinaryFile<T, AllocatorSystem>::BinaryFile;
	};
}
