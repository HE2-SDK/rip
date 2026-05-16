#pragma once
#include <rip/models/raw.h>
#include <rip/binary/stream.h>
#include <rip/binary/accessors/binary-stream.h>
#include <rip/binary/serialization2/binary.h>
#include "forwards.h"

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem, typename AddrType, std::endian endianness, bool byteswap_offsets, bool relative_offsets>
	struct binary<models::Raw<Refl, AllocatorSystem, AddrType, endianness, byteswap_offsets, relative_offsets>> {
		inline static void save(auto& backend, const models::Raw<Refl, AllocatorSystem, AddrType, endianness, byteswap_offsets, relative_offsets>& model) {
			rip::binary::mem_istream mis{ model.data };
			rip::binary::binary_istream<decltype(mis), AddrType, byteswap_offsets, relative_offsets> bis{ mis, endianness };
			typename rip::binary::accessors::binary_istream<decltype(bis)>::template ValueAccessor<Refl> acc{ bis, Refl{} };

			rip::binary::serializeBinaryToBinaryStream(backend, acc);
		}

		inline static models::Raw<Refl, AllocatorSystem, AddrType, endianness, byteswap_offsets, relative_offsets> load(auto& backend) {
			typename rip::binary::accessors::binary_istream<std::decay_t<decltype(backend)>>::template ValueAccessor<Refl> acc{ backend, Refl{} };

			return { rip::binary::serializeBinaryToAllocatorSystemBuffer<AllocatorSystem, AddrType, endianness, byteswap_offsets, relative_offsets>(acc) };
		}
	};
}
