#pragma once
#include <rip/models/mirage/v2.h>
#include <rip/binary/containers/mirage/v2.h>
#include <rip/serialization/binary/raw.h>

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem>
	struct binary<models::MirageContainerV2<Refl, AllocatorSystem>> {
	private:
		inline static void saveBranchNode(auto& nodeStream, const models::MirageContainerV2<Refl, AllocatorSystem>::Node& node) {
			for (size_t i = 0; i < node.children.size(); i++) {
				auto& child = node.children[i];

				if (child.children.size() == 0) {
					auto childStream = nodeStream.add_leaf_node(child.name, child.value, i == node.children.size() - 1);

					if (child.content.has_value())
						binary<typename models::MirageContainerV2<Refl, AllocatorSystem>::ContextsModel>::save(childStream, child.content.value());
				}
				else {
					auto childStream = nodeStream.add_branch_node(child.name, child.value, i == node.children.size() - 1);

					saveBranchNode(childStream, child);
				}
			}
		};

		inline static typename models::MirageContainerV2<Refl, AllocatorSystem>::Node loadBranchNode(auto& nodeReader) {
			std::vector<typename models::MirageContainerV2<Refl, AllocatorSystem>::Node> children{};

			nodeReader.for_each_child([&children](auto& child) {
				if (child.is_leaf())
					children.push_back({ child.header.magic, child.header.value, child.header.magic == "Contexts" ? std::make_optional(binary<typename models::MirageContainerV2<Refl, AllocatorSystem>::ContextsModel>::load(child.get_stream())) : std::nullopt, {} });
				else
					children.push_back(loadBranchNode(child));
			});

			return { nodeReader.header.magic, nodeReader.header.value, std::nullopt, std::move(children) };
		};

	public:
		inline static void save(auto& stream, const models::MirageContainerV2<Refl, AllocatorSystem>& model) {
			rip::binary::containers::mirage::v2::MirageResourceImageWriter writer{ stream };

			// MRIW assumes the root is a branch node.
			auto root = writer.add_root_node(model.root.name, model.root.value);

			saveBranchNode(root, model.root);
		}

		inline static models::MirageContainerV2<Refl, AllocatorSystem> load(auto& stream) {
			rip::binary::containers::mirage::v2::MirageResourceImageReader reader{ stream };

			auto root = reader.get_root_node();

			return { loadBranchNode(root) };
		}
	};
}
