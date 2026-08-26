#pragma once
#include <config.h>
#include "resource-table.h"

namespace rip::cli::convert {
	template<typename R, strlit V>
	void convertNamedVersion(const Config& config);
}
