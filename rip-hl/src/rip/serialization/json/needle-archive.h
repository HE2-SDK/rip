#pragma once
#include <rip/models/needle-archive.h>
#include "mirage.h"

namespace rip::serialization {
	template<typename AllocatorSystem, bool terrain>
	struct json<models::NeedleArchive<AllocatorSystem, terrain>> {
		inline static yyjson_mut_val* save(yyjson_mut_doc* doc, const models::NeedleArchive<AllocatorSystem, terrain>& model) {
			auto* result = yyjson_mut_arr(doc);

			for (auto& chunk : model.chunks) {
				std::visit([&](auto& content) {
					auto* obj = yyjson_mut_arr_add_obj(doc, result);

					yyjson_mut_obj_add_str(doc, obj, "type", models::NeedleArchive<AllocatorSystem, terrain>::template chunk_type_id_v<std::decay_t<decltype(content)>>);
					yyjson_mut_obj_add_strcpy(doc, obj, "name", chunk.name.c_str());
					yyjson_mut_obj_add(doc, obj, "content", json<std::decay_t<decltype(content)>>::save(doc, content));
				}, chunk.content);
			}

			return result;
		}

		inline static models::NeedleArchive<AllocatorSystem, terrain> load(yyjson_doc* doc, yyjson_val* value) {
			std::vector<typename models::NeedleArchive<AllocatorSystem, terrain>::Chunk> chunks{};

			if (!yyjson_is_arr(value))
				throw new std::runtime_error{ "needle archive container parsing: root value must be an array" };

			size_t i, max;
			yyjson_val* item;
			yyjson_arr_foreach(value, i, max, item) {
				auto* type = yyjson_obj_get(item, "type");
				auto* name = yyjson_obj_get(item, "name");
				auto* content = yyjson_obj_get(item, "content");

				if (type == nullptr || !yyjson_is_str(type))
					throw new std::runtime_error{ "needle archive container parsing: `type` property must be a string" };

				if (name == nullptr || !yyjson_is_str(name))
					throw new std::runtime_error{ "needle archive container parsing: `name` property must be a string" };

				if (content == nullptr || !yyjson_is_obj(content))
					throw new std::runtime_error{ "needle archive container parsing: `content` property must be an object" };

				auto* typeStr = yyjson_get_str(type);
				auto* nameStr = yyjson_get_str(name);

				if (!strcmp(typeStr, models::NeedleArchive<AllocatorSystem, terrain>::template chunk_type_id_v<typename models::NeedleArchive<AllocatorSystem, terrain>::ModelV5Model>))
					chunks.emplace_back(nameStr, json<typename models::NeedleArchive<AllocatorSystem, terrain>::ModelV5Model>::load(doc, content));
				else if (!strcmp(typeStr, models::NeedleArchive<AllocatorSystem, terrain>::template chunk_type_id_v<typename models::NeedleArchive<AllocatorSystem, terrain>::LODInfoV1Model>))
					chunks.emplace_back(nameStr, json<typename models::NeedleArchive<AllocatorSystem, terrain>::LODInfoV1Model>::load(doc, content));
				else
					throw std::runtime_error{ "Unknown NEDARCV1 chunk!" };
			}

			return { std::move(chunks) };
		}
	};
}
