#pragma once
#include <rip/models/binary-file/v2.h>
#include <rip/binary/containers/binary-file/v2.h>
#include <rip/serialization/binary/raw.h>

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem>
	struct binary<models::BinaryFileV2<Refl, AllocatorSystem>> {
		inline static void save(auto& stream, const models::BinaryFileV2<Refl, AllocatorSystem>& model) {
			rip::binary::containers::binary_file::v2::BinaryFileWriter writer{ stream };

			auto chunk = writer.addDataChunk();

			binary<rip::models::Raw<Refl, AllocatorSystem>>::save(chunk, model.data);
		}

		inline static models::BinaryFileV2<Refl, AllocatorSystem> load(auto& stream) {
			rip::binary::containers::binary_file::v2::BinaryFileReader reader{ stream };

			auto chunk = reader.getNextDataChunk();

			return { binary<rip::models::Raw<Refl, AllocatorSystem>>::load(chunk) };
		}
	};
}
