#pragma once
#include <rip/models/mirage/v1.h>
#include <rip/binary/containers/mirage/v1.h>
#include <rip/serialization/binary/raw.h>

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem>
	struct binary<models::MirageContainerV1<Refl, AllocatorSystem>> {
		inline static void save(auto& stream, const models::MirageContainerV1<Refl, AllocatorSystem>& model) {
			rip::binary::containers::mirage::v1::MirageResourceImageWriter writer{ stream };

			auto data = writer.add_data(model.version);

			binary<typename models::MirageContainerV1<Refl, AllocatorSystem>::ContextsModel>::save(data, model.content);
		}

		inline static models::MirageContainerV1<Refl, AllocatorSystem> load(auto& stream) {
			rip::binary::containers::mirage::v1::MirageResourceImageReader reader{ stream };

			auto data = reader.get_data();

			return { reader.header.version, binary<typename models::MirageContainerV1<Refl, AllocatorSystem>::ContextsModel>::load(data) };
		}
	};
}
