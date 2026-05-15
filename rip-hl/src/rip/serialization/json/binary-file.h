#pragma once
#include <rip/models/binary-file.h>
#include "raw.h"

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem>
	struct json<models::BinaryFileV2<Refl, AllocatorSystem>> {
		inline static yyjson_mut_val* save(yyjson_mut_doc* doc, const models::BinaryFileV2<Refl, AllocatorSystem>& model) {
			return json<models::Raw<Refl, AllocatorSystem>>::save(doc, model.data);
		}

		inline static models::BinaryFileV2<Refl, AllocatorSystem> load(yyjson_doc* doc, yyjson_val* value) {
			return { json<models::Raw<Refl, AllocatorSystem>>::load(doc, value) };
		}
	};
}
