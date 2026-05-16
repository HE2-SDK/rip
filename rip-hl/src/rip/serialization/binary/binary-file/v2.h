#pragma once
#include <rip/models/binary-file/v2.h>
#include <rip/binary/containers/binary-file/v2.h>
#include <rip/serialization/binary/raw.h>

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem>
	struct binary<models::BinaryFileV2<Refl, AllocatorSystem>> {
		inline static void save(auto& stream, const models::BinaryFileV2<Refl, AllocatorSystem>& model) {
			// We probably want to revert to BFW taking a binary_ostream but avoiding that refactor atm by unpacking the raw stream here.
			// Maybe endianness should also just not be type level.
			rip::binary::containers::binary_file::v2::BinaryFileWriter<typename std::decay_t<decltype(stream)>::RawStreamType, typename std::decay_t<decltype(stream)>::AddrType, std::decay_t<decltype(stream)>::endianness> writer{stream.get_raw_stream()};

			auto chunk = writer.addDataChunk();

			binary<rip::models::Raw<Refl, AllocatorSystem>>::save(chunk, model.data);
		}

		inline static models::BinaryFileV2<Refl, AllocatorSystem> load(auto& stream) {
			// We probably want to revert to BFW taking a binary_ostream but avoiding that refactor atm by unpacking the raw stream here.
			// Maybe endianness should also just not be type level.
			rip::binary::containers::binary_file::v2::BinaryFileReader<typename std::decay_t<decltype(stream)>::RawStreamType, typename std::decay_t<decltype(stream)>::AddrType> reader{ stream.get_raw_stream() };

			auto chunk = reader.getNextDataChunk();

			return { binary<rip::models::Raw<Refl, AllocatorSystem>>::load(chunk) };
		}
	};
}
