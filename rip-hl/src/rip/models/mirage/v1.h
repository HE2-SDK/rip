#pragma once
#include <rip/models/raw.h>

namespace rip::models {
	template<typename ContextType, typename AllocatorSystem>
	struct MirageContainerV1 {
		using ContextsModel = Raw<ContextType, AllocatorSystem>;

		unsigned int version{};
		ContextsModel content{};
	};
}
