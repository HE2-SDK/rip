/*
 * We're using this to split up the converters,
 * because the compiler can't handle the amount of types otherwise.
 */

#include "convert-impl.h"
#include "convert-common.h"

namespace rip::cli::convert {
	template<typename R, strlit V>
	void convertNamedVersion(const Config& config) {
		convertVersion<resources::find_version_t<V, R>>(config);
	}

	template void convertNamedVersion<resources::${RIP_RESOURCE}, "${RIP_VERSION}">(const Config& config);
}
