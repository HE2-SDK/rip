#pragma once
#include <rip/models/binary-file/v2.h>
#include <rip/serialization/json/raw.h>

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem, bool arrayVectors>
	struct json<models::BinaryFileV2<Refl, AllocatorSystem>, arrayVectors> {
		inline static yyjson_mut_val* save(yyjson_mut_doc* doc, const models::BinaryFileV2<Refl, AllocatorSystem>& model) {
			return json<models::Raw<Refl, AllocatorSystem>, arrayVectors>::save(doc, model.data);
		}

		inline static models::BinaryFileV2<Refl, AllocatorSystem> load(yyjson_doc* doc, yyjson_val* value) {
			return { json<models::Raw<Refl, AllocatorSystem>, arrayVectors>::load(doc, value) };
		}
	};
}
