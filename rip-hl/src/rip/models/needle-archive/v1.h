#pragma once
#include <ucsl-reflection/reflections/resources/lodinfo/v1.h>
#include <ucsl-reflection/reflections/resources/model/v5.h>
#include <variant>
#include <rip/models/raw.h>
#include <rip/models/mirage/v2.h>

namespace rip::models {
	template<typename AllocatorSystem, bool terrain = false>
	struct NeedleArchiveV1 {
		using LODInfoV1Model = Raw<ucsl::resources::lodinfo::v1::reflections::LODInfo, AllocatorSystem>;
		using ModelV5Model = MirageContainerV2<std::conditional_t<terrain, ucsl::resources::model::v5::reflections::TerrainModelContexts, ucsl::resources::model::v5::reflections::ModelContexts>, AllocatorSystem>;

		template<typename M> struct chunk_type_id;
		template<> struct chunk_type_id<LODInfoV1Model> { static constexpr const char* value = "NEDLDIV1"; };
		template<> struct chunk_type_id<ModelV5Model> { static constexpr const char* value = "NEDMDLV5"; };
		template<typename M> static constexpr const char* chunk_type_id_v = chunk_type_id<M>::value;

		struct Chunk {
			std::string name{};
			std::variant<LODInfoV1Model, ModelV5Model> content{};
		};

		std::vector<Chunk> chunks{};
	};
}
