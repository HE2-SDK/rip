#pragma once
#include <rip/models/raw.h>
#include <variant>
#include <ucsl/magic.h>

namespace rip::models {
	template<typename ContextType, typename AllocatorSystem>
	struct MirageContainerV2 {
		using ContextsModel = Raw<ContextType, AllocatorSystem>;

		struct Node {
			ucsl::magic_t<8> name{};
			unsigned int value{};
			std::optional<ContextsModel> content{};
			std::vector<Node> children{};
		};

		Node root{};
	};
}
