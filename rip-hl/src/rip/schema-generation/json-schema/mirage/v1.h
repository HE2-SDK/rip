#pragma once
#include <rip/models/mirage/v1.h>
#include <rip/schema-generation/json/raw.h>

namespace rip::schema_generation {
	template<typename Refl, typename AllocatorSystem>
	struct json_schema<models::MirageContainerV1<Refl, AllocatorSystem>> {
		inline static yyjson_mut_val* generate(yyjson_mut_doc* doc) {
			auto* obj = yyjson_mut_obj(doc);
			auto* props = yyjson_mut_obj_add_obj(doc, obj, "properties");

			yyjson_mut_obj_add_str(doc, props, "version", "number");
			yyjson_mut_obj_add_val(doc, props, "content", json_schema<typename models::MirageContainerV1<Refl, AllocatorSystem>::ContextsModel>::generate(doc));

			return obj;
		}
	};
}
