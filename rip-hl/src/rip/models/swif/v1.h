#pragma once
#include <rip/models/raw.h>

namespace rip::models {
	template<typename T, typename AllocatorSystem>
	struct SWIFV1 {
		Raw<T, AllocatorSystem> project{};

		inline SWIFV1() {}
		inline SWIFV1(const SWIFV1<T, AllocatorSystem>& other) = delete;
		inline SWIFV1(Raw<T, AllocatorSystem>&& project) : project{ std::move(project) } {}
		inline SWIFV1(SWIFV1<T, AllocatorSystem>&& other) : project{ std::move(other.project) } {}
	};
}
