#pragma once
#include <rip/models/mirage/v2.h>
#include <rip/serialization/json/raw.h>

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem, bool arrayVectors>
	struct json<models::MirageContainerV2<Refl, AllocatorSystem>, arrayVectors> {
	private:
		inline static yyjson_mut_val* saveNode(yyjson_mut_doc* doc, const typename models::MirageContainerV2<Refl, AllocatorSystem>::Node& node) {
			auto* obj = yyjson_mut_obj(doc);

			yyjson_mut_obj_add_strncpy(doc, obj, "name", std::string_view{ node.name }.data(), 8);
			yyjson_mut_obj_add_uint(doc, obj, "value", node.value);

			if (node.children.size() > 0) {
				auto* children = yyjson_mut_obj_add_arr(doc, obj, "children");

				for (auto& child : node.children)
					yyjson_mut_arr_add_val(children, saveNode(doc, child));
			}
			else if (node.content.has_value())
				yyjson_mut_obj_add_val(doc, obj, "content", json<typename models::MirageContainerV2<Refl, AllocatorSystem>::ContextsModel>::save(doc, node.content.value()));

			return obj;
		}

		inline static typename models::MirageContainerV2<Refl, AllocatorSystem>::Node loadNode(yyjson_doc* doc, yyjson_val* node) {
			if (!yyjson_is_obj(node))
				throw new std::runtime_error{ "MIRAGE v2 container parsing: node must be an object" };

			auto* name = yyjson_obj_get(node, "name");
			auto* value = yyjson_obj_get(node, "value");
			auto* content = yyjson_obj_get(node, "content");
			auto* children = yyjson_obj_get(node, "children");

			if (name == nullptr || !yyjson_is_str(name))
				throw new std::runtime_error{ "MIRAGE v2 container parsing: `name` property must be a string" };

			if (value == nullptr || !yyjson_is_uint(value))
				throw new std::runtime_error{ "MIRAGE v2 container parsing: `value` property must be an unsigned integer" };

			if (children != nullptr) {
				if (!yyjson_is_arr(children))
					throw new std::runtime_error{ "MIRAGE v2 container parsing: `children` property must be an array" };

				std::vector<typename models::MirageContainerV2<Refl, AllocatorSystem>::Node> childVec{};

				size_t i, max;
				yyjson_val* item;
				yyjson_arr_foreach(children, i, max, item) {
					childVec.push_back(loadNode(doc, item));
				}

				return { std::string_view{ yyjson_get_str(name) }, static_cast<unsigned int>(yyjson_get_uint(value)), {}, std::move(childVec) };
			}
			else
				return { std::string_view{ yyjson_get_str(name) }, static_cast<unsigned int>(yyjson_get_uint(value)), content != nullptr ? std::make_optional(json<typename models::MirageContainerV2<Refl, AllocatorSystem>::ContextsModel>::load(doc, content)) : std::nullopt, {}};
		}

	public:
		inline static yyjson_mut_val* save(yyjson_mut_doc* doc, const models::MirageContainerV2<Refl, AllocatorSystem>& model) {
			return saveNode(doc, model.root);
		}

		inline static models::MirageContainerV2<Refl, AllocatorSystem> load(yyjson_doc* doc, yyjson_val* value) {
			return { loadNode(doc, value) };
		}
	};
}
