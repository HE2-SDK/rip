#pragma once
#include "raw.h"
#include <variant>

namespace rip::models {
	template<typename ContextType, typename AllocatorSystem>
	struct MirageContainer {
		using ContextsModel = Raw<ContextType, AllocatorSystem>;

		struct Node {
			std::string name{};
			unsigned int value{};
			std::optional<ContextsModel> content{};
			std::vector<Node> children{};
		};

		Node root{};
	};
}
