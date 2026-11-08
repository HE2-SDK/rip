#pragma once
#include <rip/models/swif/v1.h>
#include <rip/binary/containers/swif/v1.h>
#include <rip/serialization/binary/raw.h>

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem>
	struct binary<models::SWIFV1<Refl, AllocatorSystem>> {
		inline static void save(auto& stream, const models::SWIFV1<Refl, AllocatorSystem>& model) {
			rip::binary::containers::swif::v1::SWIFWriter writer{ stream };

			rip::binary::mem_istream mis{ model.project.data };
			rip::binary::binary_istream<decltype(mis), size_t, true, false> bis{ mis, std::endian::native };
			typename rip::binary::accessors::binary_istream<decltype(bis)>::template ValueAccessor<Refl> acc{ bis, Refl{} };

			rip::binary::BinarySerializer serializer{ writer.stream };

			unsigned short textureListCount = acc.as_structure().template get_field<"textureListCount">().as_primitive().template as<unsigned short>();
			auto textureLists = (*acc.as_structure().template get_field<"textureLists">().as_pointer());

			if (textureListCount > 0) {
				auto tlChunk = writer.add_texture_list_chunk(textureListCount);

				serializer.process(textureLists);
			}

			auto prChunk = writer.add_project_chunk();

			serializer.process(acc);
		}

		inline static models::SWIFV1<Refl, AllocatorSystem> load(auto& stream) {
			rip::binary::containers::swif::v1::SWIFReader reader{ stream };

			models::SWIFV1<Refl, AllocatorSystem> result{};

			reader.for_each_chunk([&](const ucsl::magic_t<4>& magic, auto& chunk) {
				if (magic != "SWPR")
					return;

				auto pos = chunk.tellg();
				rip::binary::containers::swif::v1::SRS_PROJECT_CHUNK_HEADER header{};
				chunk.read(header);
				chunk.seekg(pos + header.startOffset - 8);

				result.project = binary<rip::models::Raw<Refl, AllocatorSystem>>::load(chunk);
			});

			return result;
		}
	};
}
