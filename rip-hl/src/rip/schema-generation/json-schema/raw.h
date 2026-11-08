#pragma once
#include <rip/models/raw.h>
#include <rip/binary/stream.h>
#include <rip/binary/accessors/binary-stream.h>
#include <rip/binary/accessors/json.h>
#include <rip/binary/schema-generation/binary.h>
#include <rip/binary/schema-generation/json.h>
#include "./forwards.h"

namespace rip::schema_generation {
	template<typename Refl, typename AllocatorSystem, typename AddrType, std::endian endianness, bool byteswap_offsets, bool relative_offsets>
	struct json_schema<models::Raw<Refl, AllocatorSystem, AddrType, endianness, byteswap_offsets, relative_offsets>> {
		inline static yyjson_mut_val* generate(yyjson_mut_doc* doc) {
			return rip::binary::generateJsonSchema(doc, Refl{});
		}
	};
}
