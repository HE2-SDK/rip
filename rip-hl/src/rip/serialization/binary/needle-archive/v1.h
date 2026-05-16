#pragma once
#include <rip/models/needle-archive/v1.h>
#include <rip/binary/containers/needle-archive/v1.h>
#include <rip/serialization/binary/mirage/v2.h>

namespace rip::serialization {
	template<typename AllocatorSystem, bool terrain>
	struct binary<models::NeedleArchiveV1<AllocatorSystem, terrain>> {
		inline static void save(auto& stream, const models::NeedleArchiveV1<AllocatorSystem, terrain>& model) {
		}

		inline static models::NeedleArchiveV1<AllocatorSystem, terrain> load(auto& stream) {
			rip::binary::containers::needle_archive::v1::NeedleArchiveV1Reader reader{ stream };

			std::vector<typename models::NeedleArchiveV1<AllocatorSystem, terrain>::Chunk> chunks{};

			reader.forEachChunk([&](auto& chunk) {
				if (chunk.header.magic == models::NeedleArchiveV1<AllocatorSystem, terrain>::template chunk_type_id_v<typename models::NeedleArchiveV1<AllocatorSystem, terrain>::ModelV5Model>)
					chunks.emplace_back(chunk.header.name, binary<typename models::NeedleArchiveV1<AllocatorSystem, terrain>::ModelV5Model>::load(chunk.stream));
				else if (chunk.header.magic == models::NeedleArchiveV1<AllocatorSystem, terrain>::template chunk_type_id_v<typename models::NeedleArchiveV1<AllocatorSystem, terrain>::LODInfoV1Model>)
					chunks.emplace_back(chunk.header.name, binary<typename models::NeedleArchiveV1<AllocatorSystem, terrain>::LODInfoV1Model>::load(chunk.stream));
				else
					throw std::runtime_error{ "Unknown NEDARCV1 chunk!" };
			});

			return { std::move(chunks) };
		}
	};
}
