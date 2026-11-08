#pragma once
#include <rip/binary/stream.h>

namespace rip::serialization {
	template<typename T>
	struct binary {
		inline static size_t measure(const T& model) {
			rip::binary::null_ostream nos{};

			toBinary<rip::binary::null_ostream, T>(nos, model);

			return nos.tellp();
		}
	};
}
