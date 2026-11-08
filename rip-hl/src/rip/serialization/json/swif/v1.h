#pragma once
#include <rip/models/swif/v1.h>
#include <rip/serialization/json/raw.h>

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem, bool arrayVectors>
	struct json<models::SWIFV1<Refl, AllocatorSystem>, arrayVectors> {
		inline static yyjson_mut_val* save(yyjson_mut_doc* doc, const models::SWIFV1<Refl, AllocatorSystem>& model) {
			return json<models::Raw<Refl, AllocatorSystem>, arrayVectors>::save(doc, model.project);
		}

		inline static models::SWIFV1<Refl, AllocatorSystem> load(yyjson_doc* doc, yyjson_val* value) {
			return { json<models::Raw<Refl, AllocatorSystem>, arrayVectors>::load(doc, value) };
		}
	};
}
