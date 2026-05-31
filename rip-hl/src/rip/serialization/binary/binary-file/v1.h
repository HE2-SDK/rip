#pragma once
#include <rip/models/binary-file/v1.h>
#include <rip/binary/containers/binary-file/v1.h>
#include <rip/serialization/binary/raw.h>

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem, bool include_bvh>
	struct binary<models::BinaryFileV1<Refl, AllocatorSystem, include_bvh>> {
		inline static void save(auto& stream, const models::BinaryFileV1<Refl, AllocatorSystem, include_bvh>& model) {
			// We probably want to revert to BFW taking a binary_ostream but avoiding that refactor atm by unpacking the raw stream here.
			// Maybe endianness should also just not be type level.
			rip::binary::containers::binary_file::v1::BinaryFileWriter<typename std::decay_t<decltype(stream)>::RawStreamType, typename std::decay_t<decltype(stream)>::AddrType, std::decay_t<decltype(stream)>::endianness, include_bvh> writer{stream.get_raw_stream()};

			auto chunk = writer.getDataChunk();

			binary<rip::models::Raw<Refl, AllocatorSystem>>::save(chunk, model.data);
		}

		inline static models::BinaryFileV1<Refl, AllocatorSystem, include_bvh> load(auto& stream) {
			// We probably want to revert to BFW taking a binary_ostream but avoiding that refactor atm by unpacking the raw stream here.
			// Maybe endianness should also just not be type level.
			rip::binary::containers::binary_file::v1::BinaryFileReader<typename std::decay_t<decltype(stream)>::RawStreamType, typename std::decay_t<decltype(stream)>::AddrType> reader{ stream.get_raw_stream() };

			auto chunk = reader.getData();

			return { binary<rip::models::Raw<Refl, AllocatorSystem>>::load(chunk) };
		}
	};
}
