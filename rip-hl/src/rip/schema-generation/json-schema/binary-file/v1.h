#pragma once
#include <rip/models/binary-file/v1.h>
#include <rip/schema-generation/json-schema/raw.h>

namespace rip::schema_generation {
	template<typename Refl, typename AllocatorSystem, bool include_bvh>
	struct json_schema<models::BinaryFileV1<Refl, AllocatorSystem, include_bvh>> {
		inline static yyjson_mut_val* generate(yyjson_mut_doc* doc) {
			return json_schema<models::Raw<Refl, AllocatorSystem>>::generate(doc);
		}
	};
}
