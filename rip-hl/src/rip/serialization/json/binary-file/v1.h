#pragma once
#include <rip/models/binary-file/v1.h>
#include <rip/serialization/json/raw.h>

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem, bool arrayVectors>
	struct json<models::BinaryFileV1<Refl, AllocatorSystem>, arrayVectors> {
		inline static yyjson_mut_val* save(yyjson_mut_doc* doc, const models::BinaryFileV1<Refl, AllocatorSystem>& model) {
			return json<models::Raw<Refl, AllocatorSystem>>::save(doc, model.data);
		}

		inline static models::BinaryFileV1<Refl, AllocatorSystem> load(yyjson_doc* doc, yyjson_val* value) {
			return { json<models::Raw<Refl, AllocatorSystem>>::load(doc, value) };
		}
	};
}
