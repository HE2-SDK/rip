#pragma once
#include <rip/models/binary-file/v2.h>
#include <rip/schema-generation/json-schema/raw.h>

namespace rip::schema_generation {
	template<typename Refl, typename AllocatorSystem>
	struct json_schema<models::BinaryFileV2<Refl, AllocatorSystem>> {
		inline static yyjson_mut_val* generate(yyjson_mut_doc* doc) {
			return json_schema<models::Raw<Refl, AllocatorSystem>>::generate(doc);
		}
	};
}
