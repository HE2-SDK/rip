#pragma once
#include <rip/models/mirage.h>
#include "raw.h"

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem>
	struct json<models::MirageContainer<Refl, AllocatorSystem>> {
		inline static yyjson_mut_val* save(yyjson_mut_doc* doc, const models::MirageContainer<Refl, AllocatorSystem>& model) {
			auto handleNode = [doc](yyjson_mut_val* parent, const typename models::MirageContainer<Refl, AllocatorSystem>::Node& node) -> yyjson_mut_val* {
				auto* obj = yyjson_mut_arr_add_obj(doc, parent);

				yyjson_mut_obj_add_strcpy(doc, obj, "name", node.name.c_str());
				yyjson_mut_obj_add_uint(doc, obj, "value", node.value.integer);

				if (node.children.size() > 0) {
					auto* children = yyjson_mut_obj_add_arr(doc, obj, "children");

					for (auto& node : node.children) {
						auto* child = yyjson_mut_arr_add_obj(doc, children);

						handleNode(child, node);
					}
				}
				else if (node.content.has_value())
					yyjson_mut_obj_add(doc, obj, "content", json<typename models::MirageContainer<Refl, AllocatorSystem>::ContextsModel>::save(doc, node.content.value()));
			};

			yyjson_mut_val* root = yyjson_mut_obj(doc);

			handleNode(root, model.root);

			return root;
		}

		inline static models::MirageContainer<Refl, AllocatorSystem> load(yyjson_doc* doc, yyjson_val* value) {
			auto handleNode = [doc](yyjson_val* node) -> typename models::MirageContainer<Refl, AllocatorSystem>::Node {
				auto* name = yyjson_obj_get(node, "name");
				auto* value = yyjson_obj_get(node, "value");
				auto* content = yyjson_obj_get(node, "content");
				auto* children = yyjson_obj_get(node, "children");

				if (name == nullptr || !yyjson_is_str(name))
					throw new std::runtime_error{ "MIRAGE container parsing: `name` property must be a string" };

				if (value == nullptr || !yyjson_is_uint(value))
					throw new std::runtime_error{ "MIRAGE container parsing: `value` property must be an unsigned integer" };

				if (children != nullptr) {
					if (!yyjson_is_arr(children))
						throw new std::runtime_error{ "MIRAGE container parsing: `children` property must be an array" };

					std::vector<typename models::MirageContainer<Refl, AllocatorSystem>::Node> childVec{};

					size_t i, max;
					yyjson_val* item;
					yyjson_arr_foreach(children, i, max, item) {
						if (!yyjson_is_obj(item))
							throw new std::runtime_error{ "MIRAGE container parsing: `children` property item must be an object" };

						childVec.push_back(handleNode(item));
					}

					return { yyjson_get_str(name), yyjson_get_uint(value), {}, std::move(childVec) };
				}
				else if (content != nullptr)
					return { yyjson_get_str(name), yyjson_get_uint(value), std::make_optional(json<typename models::MirageContainer<Refl, AllocatorSystem>::ContextsModel>::load()), {} };
			};

			return { handleNode(value) };
		}
	};
}
