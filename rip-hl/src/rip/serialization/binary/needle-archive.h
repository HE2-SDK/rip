#pragma once
#include <rip/models/needle-archive.h>
#include <rip/binary/containers/needle-archive/v1.h>
#include "mirage.h"

namespace rip::serialization {
	template<typename AllocatorSystem, bool terrain>
	struct binary<models::NeedleArchive<AllocatorSystem, terrain>> {
		inline static void save(auto& stream, const models::NeedleArchive<AllocatorSystem, terrain>& model) {
		}

		inline static models::NeedleArchive<AllocatorSystem, terrain> load(auto& stream) {
			rip::binary::containers::needle_archive::v1::NeedleArchiveReader reader{ stream };

			std::vector<typename models::NeedleArchive<AllocatorSystem, terrain>::Chunk> chunks{};

			reader.forEachChunk([&](auto& chunk) {
				if (chunk.header.magic == models::NeedleArchive<AllocatorSystem, terrain>::template chunk_type_id_v<typename models::NeedleArchive<AllocatorSystem, terrain>::ModelV5Model>)
					chunks.emplace_back(chunk.header.name, binary<typename models::NeedleArchive<AllocatorSystem, terrain>::ModelV5Model>::load(chunk.stream));
				else if (chunk.header.magic == models::NeedleArchive<AllocatorSystem, terrain>::template chunk_type_id_v<typename models::NeedleArchive<AllocatorSystem, terrain>::LODInfoV1Model>)
					chunks.emplace_back(chunk.header.name, binary<typename models::NeedleArchive<AllocatorSystem, terrain>::LODInfoV1Model>::load(chunk.stream));
				else
					throw std::runtime_error{ "Unknown NEDARCV1 chunk!" };
			});

			return { std::move(chunks) };
		}
	};
}
