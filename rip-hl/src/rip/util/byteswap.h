#pragma once
#include <iostream>
#include <bit>
#include <algorithm>
#include <ucsl/math.h>
#include <ucsl/colors.h>
#include <ucsl/object-id.h>

#ifndef __cpp_lib_byteswap
namespace std {
	// Reference implementation from cppreference.com
	template<integral T> constexpr T byteswap(T value) noexcept
	{
		static_assert(has_unique_object_representations_v<T>, "T may not have padding bits");
		auto value_representation = bit_cast<array<byte, sizeof(T)>>(value);
		ranges::reverse(value_representation);
		return bit_cast<T>(value_representation);
	}
}
#endif

namespace rip::util {
	template<typename T> inline T byteswap(T value) noexcept { static_assert("invalid byteswap"); }
	template<std::integral T> inline T byteswap(T value) noexcept { return std::byteswap(value); }
	template<> inline unsigned long long byteswap(unsigned long long value) noexcept { return _byteswap_uint64(value); }
	template<> inline unsigned int byteswap(unsigned int value) noexcept { return _byteswap_ulong(value); }
	template<> inline unsigned short byteswap(unsigned short value) noexcept { return _byteswap_ushort(value); }
	template<> inline double byteswap(double value) noexcept { return std::bit_cast<double>(_byteswap_uint64(std::bit_cast<unsigned long long>(value))); }
	template<> inline float byteswap(float value) noexcept { return std::bit_cast<float>(_byteswap_ulong(std::bit_cast<unsigned int>(value))); }

	template<std::integral T>
	T byteswap_to_native(std::endian endianness, T value) noexcept {
		return std::endian::native != endianness ? byteswap(value) : value;
	}

	template<typename T> inline void byteswap_deep(T& value) noexcept { value.byteswap_deep(); }
	template<std::integral T> inline void byteswap_deep(T& value) noexcept { value = byteswap(value); }
	template<std::floating_point T> inline void byteswap_deep(T& value) noexcept { value = byteswap(value); }
	// TODO: use reflection here.
	template<> inline void byteswap_deep(ucsl::math::Vector2& value) noexcept {
		byteswap_deep(value.x());
		byteswap_deep(value.y());
	}
	template<> inline void byteswap_deep(ucsl::math::Vector3& value) noexcept {
		byteswap_deep(value.x());
		byteswap_deep(value.y());
		byteswap_deep(value.z());
	}
	template<> inline void byteswap_deep(ucsl::math::Vector4& value) noexcept {
		byteswap_deep(value.x());
		byteswap_deep(value.y());
		byteswap_deep(value.z());
		byteswap_deep(value.w());
	}
	template<> inline void byteswap_deep(ucsl::math::Quaternion& value) noexcept {
		byteswap_deep(value.x());
		byteswap_deep(value.y());
		byteswap_deep(value.z());
		byteswap_deep(value.w());
	}
	template<> inline void byteswap_deep(ucsl::math::Matrix34& value) noexcept {
		for (size_t i = 0; i < value.rows(); i++)
			for (size_t j = 0; j < value.cols(); j++)
				byteswap_deep(value(i, j));
	}
	template<> inline void byteswap_deep(ucsl::math::Matrix44& value) noexcept {
		for (size_t i = 0; i < value.rows(); i++)
			for (size_t j = 0; j < value.cols(); j++)
				byteswap_deep(value(i, j));
	}
	template<> inline void byteswap_deep(ucsl::math::Position& value) noexcept {
		byteswap_deep(value.x());
		byteswap_deep(value.y());
		byteswap_deep(value.z());
	}
	template<> inline void byteswap_deep(ucsl::math::Rotation& value) noexcept {
		byteswap_deep(value.x());
		byteswap_deep(value.y());
		byteswap_deep(value.z());
		byteswap_deep(value.w());
	}
	template<typename T, ucsl::colors::ChannelOrder order> inline void byteswap_deep(ucsl::colors::Color<T, order>& value) noexcept {
		byteswap_deep(value.r);
		byteswap_deep(value.g);
		byteswap_deep(value.b);
		byteswap_deep(value.a);
	}
	template<> inline void byteswap_deep(ucsl::objectids::ObjectIdV1& value) noexcept {
		byteswap_deep(value.id);
	}
	template<> inline void byteswap_deep(ucsl::objectids::ObjectIdV2& value) noexcept {
		byteswap_deep(value.objectId);
		byteswap_deep(value.groupId);
	}

	template<typename T>
	inline void byteswap_deep_to_native(std::endian endianness, T& value) noexcept {
		if (std::endian::native != endianness)
			  byteswap_deep(value);
	}
}
