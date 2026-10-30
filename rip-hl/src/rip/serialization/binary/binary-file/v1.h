#pragma once
#include <rip/models/binary-file/v1.h>
#include <rip/binary/containers/binary-file/v1.h>
#include <rip/serialization/binary/raw.h>

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem, bool include_bvh>
	struct binary<models::BinaryFileV1<Refl, AllocatorSystem, include_bvh>> {
		inline static void save(auto& stream, const models::BinaryFileV1<Refl, AllocatorSystem, include_bvh>& model) {
			rip::binary::containers::binary_file::v1::BinaryFileWriter<std::decay_t<decltype(stream)>, include_bvh> writer{ stream };

			auto chunk = writer.getDataChunk();

			binary<rip::models::Raw<Refl, AllocatorSystem>>::save(chunk, model.data);
		}

		inline static models::BinaryFileV1<Refl, AllocatorSystem, include_bvh> load(auto& stream) {
			rip::binary::containers::binary_file::v1::BinaryFileReader reader{ stream };

			auto chunk = reader.getData();

			return { binary<rip::models::Raw<Refl, AllocatorSystem>>::load(chunk) };
		}
	};
}
