#pragma once
#include <optional>

namespace rip::binary {
	enum class AddressingMode {
		AM32,
		AM64
	};

	template<typename U> struct offset_t : public std::optional<size_t> {
	public:
		using std::optional<size_t>::optional;
	};

	enum class size_val_t : size_t {};
	enum class ptrdiff_val_t : ptrdiff_t {};
}
