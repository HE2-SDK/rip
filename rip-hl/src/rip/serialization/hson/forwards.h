#pragma once

namespace rip::serialization {
	// We need to pass GameInterface because we need to get the sizes of RflClasses for ComponentData.
	// If we ever in the future add the ability in simplerfl to have calculated fields with the size of another field, we could drop this.
	template<typename GameInterface, typename T>
	struct hson;
}
