#pragma once
#include <rip/models/mirage.h>
#include <rip/binary/containers/mirage/v2.h>
#include "raw.h"

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem>
	struct binary<models::MirageContainer<Refl, AllocatorSystem>> {
		inline static void save(auto& stream, const models::MirageContainer<Refl, AllocatorSystem>& model) {
			auto handleBranchNode = [](rip::binary::containers::mirage::v2::MirageResourceImageWriter<std::decay_t<decltype(stream)>>::branch_node_ostream& nodeStream, const models::MirageContainer<Refl, AllocatorSystem>::Node& node) -> void {
				for (size_t i = 0; i < node.children.size(); i++) {
					auto& child = node.children[i];

					if (child.children.size() == 0) {
						auto childStream = nodeStream.add_leaf_node(child.name, child.value, i == node.children.size() - 1);

						if (node.content.has_value())
							binary<typename models::MirageContainer<Refl, AllocatorSystem>::ContextsModel>::save(childStream, node.content.value());
					}
					else {
						auto childStream = nodeStream.add_branch_node(child.name, child.value, i == node.children.size() - 1);

						handleBranchNode(childStream, child);
					}
				}
			};

			rip::binary::containers::mirage::v2::MirageResourceImageWriter writer{ stream };

			// MRIW assumes the root is a branch node.
			auto root = writer.add_root_node(model.root.name, model.root.value);

			handleBranchNode(root, model.root);
		}

		inline static models::MirageContainer<Refl, AllocatorSystem> load(auto& stream) {
			auto handleBranchNode = [](rip::binary::containers::mirage::v2::MirageResourceImageReader<std::decay_t<decltype(stream)>>::NodeReader& nodeReader) -> typename models::MirageContainer<Refl, AllocatorSystem>::Node {
				std::vector<typename models::MirageContainer<Refl, AllocatorSystem>::Node> children{};

				nodeReader.for_each_child([&children](auto& child) {
					if (child.is_leaf)
						children.emplace_back(child.header.magic, child.header.version, child.header.magic == "Contexts" ? std::make_optional(binary<typename models::MirageContainer<Refl, AllocatorSystem>::ContextsModel>::load(child)) : std::nullopt, {});
					else
						children.emplace_back(handleBranchNode(child));
				});

				return { nodeReader.header.magic, nodeReader.header.version, std::nullopt, std::move(children) };
			};

			rip::binary::containers::mirage::v2::MirageResourceImageReader reader{ stream };

			auto root = reader.get_root_node();

			return { handleBranchNode(root) };
		}
	};
}
