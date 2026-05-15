#pragma once
#include <rip/models/raw.h>
#include <rip/binary/stream.h>
#include <rip/binary/accessors/binary-stream.h>
#include <rip/binary/accessors/json.h>
#include <rip/binary/serialization2/binary.h>
#include <rip/binary/serialization2/json.h>
#include "./forwards.h"

namespace rip::serialization {
	template<typename Refl, typename AllocatorSystem, typename AddrType, std::endian endianness, bool byteswap_offsets, bool relative_offsets>
	struct json<models::Raw<Refl, AllocatorSystem, AddrType, endianness, byteswap_offsets, relative_offsets>> {
		inline static yyjson_mut_val* save(yyjson_mut_doc* doc, const models::Raw<Refl, AllocatorSystem, AddrType, endianness, byteswap_offsets, relative_offsets>& model) {
			rip::binary::mem_istream mis{ model.data };
			rip::binary::binary_istream<decltype(mis), AddrType> bis{ mis };
			typename rip::binary::accessors::template binary_istream<decltype(bis)>::template ValueAccessor<Refl> acc{ bis, Refl{} };

			return rip::binary::serializeJson<true>(doc, acc);
		}

		inline static models::Raw<Refl, AllocatorSystem, AddrType, endianness, byteswap_offsets, relative_offsets> load(yyjson_doc* doc, yyjson_val* value) {
			typename rip::binary::accessors::template json<false>::template ValueAccessor<Refl> acc{ doc, Refl{} };

			return { rip::binary::serializeBinaryToAllocatorSystemBuffer<AllocatorSystem, AddrType, endianness, byteswap_offsets, relative_offsets>(acc) };
		}
	};
}
