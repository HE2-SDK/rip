#pragma once
#include <rip/models/mirage/v1.h>
#include <rip/serialization/json/raw.h>

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem, bool arrayVectors>
	struct json<models::MirageContainerV1<Refl, AllocatorSystem>, arrayVectors> {
		inline static yyjson_mut_val* save(yyjson_mut_doc* doc, const models::MirageContainerV1<Refl, AllocatorSystem>& model) {
			auto* obj = yyjson_mut_obj(doc);

			yyjson_mut_obj_add_uint(doc, obj, "version", model.version);
			yyjson_mut_obj_add_val(doc, obj, "content", json<typename models::MirageContainerV1<Refl, AllocatorSystem>::ContextsModel, arrayVectors>::save(doc, model.content));

			return obj;
		}

		inline static models::MirageContainerV1<Refl, AllocatorSystem> load(yyjson_doc* doc, yyjson_val* value) {
			if (!yyjson_is_obj(value))
				throw new std::runtime_error{ "MIRAGE v1 container parsing: root must be an object" };

			auto* version = yyjson_obj_get(value, "version");
			auto* content = yyjson_obj_get(value, "content");

			if (version == nullptr || !yyjson_is_uint(version))
				throw new std::runtime_error{ "MIRAGE v1 container parsing: `version` property must be an unsigned integer" };

			if (content == nullptr)
				throw new std::runtime_error{ "MIRAGE v1 container parsing: `content` property must exist" };

			return { static_cast<unsigned int>(yyjson_get_uint(version)), json<typename models::MirageContainerV1<Refl, AllocatorSystem>::ContextsModel, arrayVectors>::load(doc, content) };
		}
	};
}
