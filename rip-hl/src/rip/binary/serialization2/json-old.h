#pragma once
#include <ucsl-reflection/reflections/basic-types.h>
#include <ucsl-reflection/traversals/types.h>
#include <ucsl-reflection/accessors/types.h>
#include <ucsl-reflection/opaque.h>
#include <yyjson.h>
#include <iomanip>
#include <sstream>
#include <rip/util/object-id-guids.h>

namespace rip::binary {
	using namespace ucsl::reflection;
	using namespace ucsl::reflection::traversals;

	template<bool arrayVectors = false>
	class SerializeJson {
	public:
		constexpr static size_t arity = 1;
		struct result_type {
			yyjson_mut_val* value{};

			result_type() {}
			result_type(yyjson_mut_val* value) : value{ value } {}
			result_type& operator|=(const result_type& other) { return *this; }
		};

		yyjson_mut_doc* doc;
		yyjson_mut_val* currentStruct{};

		SerializeJson(yyjson_mut_doc* doc) : doc{ doc } {}

		template<std::integral T, std::enable_if_t<std::is_signed_v<T>, bool> = true>
		result_type visit_primitive_data(const T& obj) {
			return yyjson_mut_sint(doc, obj);
		}

		template<std::integral T, std::enable_if_t<!std::is_signed_v<T>, bool> = true>
		result_type visit_primitive_data(const T& obj) {
			return yyjson_mut_uint(doc, obj);
		}

		result_type visit_primitive_data(const float& obj) {
			return yyjson_mut_float(doc, obj);
		}

		result_type visit_primitive_data(const double& obj) {
			return yyjson_mut_real(doc, obj);
		}

		result_type visit_primitive_data(const bool& obj) {
			return yyjson_mut_bool(doc, obj);
		}

		result_type visit_primitive_data(const ucsl::math::Vector2& obj) {
			if constexpr (arrayVectors) {
				yyjson_mut_val* res = yyjson_mut_arr(doc);
				for (size_t i = 0; i < obj.rows(); i++)
					for (size_t j = 0; j < obj.cols(); j++)
						yyjson_mut_arr_add_float(doc, res, obj(i, j));
				return res;
			}
			else {
				yyjson_mut_val* res = yyjson_mut_obj(doc);
				yyjson_mut_obj_add_float(doc, res, "x", obj.x());
				yyjson_mut_obj_add_float(doc, res, "y", obj.y());
				return res;
			}
		}

		result_type visit_primitive_data(const ucsl::math::Vector3& obj) {
			if constexpr (arrayVectors) {
				yyjson_mut_val* res = yyjson_mut_arr(doc);
				for (size_t i = 0; i < obj.rows(); i++)
					for (size_t j = 0; j < obj.cols(); j++)
						yyjson_mut_arr_add_float(doc, res, obj(i, j));
				return res;
			}
			else {
				yyjson_mut_val* res = yyjson_mut_obj(doc);
				yyjson_mut_obj_add_float(doc, res, "x", obj.x());
				yyjson_mut_obj_add_float(doc, res, "y", obj.y());
				yyjson_mut_obj_add_float(doc, res, "z", obj.z());
				return res;
			}
		}

		result_type visit_primitive_data(const ucsl::math::Position& obj) {
			if constexpr (arrayVectors) {
				yyjson_mut_val* res = yyjson_mut_arr(doc);
				for (size_t i = 0; i < obj.rows(); i++)
					for (size_t j = 0; j < obj.cols(); j++)
						yyjson_mut_arr_add_float(doc, res, obj(i, j));
				return res;
			}
			else {
				yyjson_mut_val* res = yyjson_mut_obj(doc);
				yyjson_mut_obj_add_float(doc, res, "x", obj.x());
				yyjson_mut_obj_add_float(doc, res, "y", obj.y());
				yyjson_mut_obj_add_float(doc, res, "z", obj.z());
				return res;
			}
		}

		result_type visit_primitive_data(const ucsl::math::Vector4& obj) {
			if constexpr (arrayVectors) {
				yyjson_mut_val* res = yyjson_mut_arr(doc);
				for (size_t i = 0; i < obj.rows(); i++)
					for (size_t j = 0; j < obj.cols(); j++)
						yyjson_mut_arr_add_float(doc, res, obj(i, j));
				return res;
			}
			else {
				yyjson_mut_val* res = yyjson_mut_obj(doc);
				yyjson_mut_obj_add_float(doc, res, "x", obj.x());
				yyjson_mut_obj_add_float(doc, res, "y", obj.y());
				yyjson_mut_obj_add_float(doc, res, "z", obj.z());
				yyjson_mut_obj_add_float(doc, res, "w", obj.w());
				return res;
			}
		}

		result_type visit_primitive_data(const ucsl::math::Quaternion& obj) {
			if constexpr (arrayVectors) {
				yyjson_mut_val* res = yyjson_mut_arr(doc);
				for (size_t i = 0; i < obj.coeffs().rows(); i++)
					for (size_t j = 0; j < obj.coeffs().cols(); j++)
						yyjson_mut_arr_add_float(doc, res, obj.coeffs()(i, j));
				return res;
			}
			else {
				yyjson_mut_val* res = yyjson_mut_obj(doc);
				yyjson_mut_obj_add_float(doc, res, "x", obj.x());
				yyjson_mut_obj_add_float(doc, res, "y", obj.y());
				yyjson_mut_obj_add_float(doc, res, "z", obj.z());
				yyjson_mut_obj_add_float(doc, res, "w", obj.w());
				return res;
			}
		}

		result_type visit_primitive_data(const ucsl::math::Matrix34& obj) {
			yyjson_mut_val* res = yyjson_mut_arr(doc);
			for (size_t i = 0; i < obj.rows(); i++)
				for (size_t j = 0; j < obj.cols(); j++)
					yyjson_mut_arr_add_float(doc, res, obj(i, j));
			return res;
		}

		result_type visit_primitive_data(const ucsl::math::Matrix44& obj) {
			yyjson_mut_val* res = yyjson_mut_arr(doc);
			for (size_t i = 0; i < obj.rows(); i++)
				for (size_t j = 0; j < obj.cols(); j++)
					yyjson_mut_arr_add_float(doc, res, obj(i, j));
			return res;
		}

		template<ucsl::colors::ChannelOrder order>
		result_type visit_primitive_data(const ucsl::colors::Color8<order>& obj) {
			yyjson_mut_val* res = yyjson_mut_obj(doc);
			yyjson_mut_obj_add_uint(doc, res, "r", obj.r);
			yyjson_mut_obj_add_uint(doc, res, "g", obj.g);
			yyjson_mut_obj_add_uint(doc, res, "b", obj.b);
			yyjson_mut_obj_add_uint(doc, res, "a", obj.a);
			return res;
		}

		template<ucsl::colors::ChannelOrder order>
		result_type visit_primitive_data(const ucsl::colors::Colorf<order>& obj) {
			yyjson_mut_val* res = yyjson_mut_obj(doc);
			yyjson_mut_obj_add_float(doc, res, "r", obj.r);
			yyjson_mut_obj_add_float(doc, res, "g", obj.g);
			yyjson_mut_obj_add_float(doc, res, "b", obj.b);
			yyjson_mut_obj_add_float(doc, res, "a", obj.a);
			return res;
		}

		result_type visit_primitive_data(const ucsl::objectids::ObjectIdV1& obj) {
			char guid[39];
			util::toGUID(obj, guid);
			return yyjson_mut_strcpy(doc, guid);
		}

		result_type visit_primitive_data(const ucsl::objectids::ObjectIdV2& obj) {
			char guid[39];
			util::toGUID(obj, guid);
			return yyjson_mut_strcpy(doc, guid);
		}

		result_type visit_primitive_data(const ucsl::strings::VariableString& obj) {
			return yyjson_mut_strcpy(doc, obj.c_str());
		}

		result_type visit_primitive_data(const char* const& obj) {
			return yyjson_mut_strcpy(doc, obj);
		}

		result_type visit_primitive_data(void* const& obj) {
			return yyjson_mut_str(doc, "TODO");
		}

		template<ucsl::reflection::accessors::PrimitiveAccessor T>
		result_type visit_primitive(T obj) {
			return obj.visit([&](auto data) {
				if constexpr (std::is_same_v<typename decltype(data.refl)::repr, const char*>)
					return visit_primitive_data(std::string{ data }.c_str());
				else
					return visit_primitive_data(typename decltype(data.refl)::repr{ data });
			});
		}

		template<typename T>
		result_type visit_enum(T obj) {
			for (auto& option : obj.refl.get_options())
				if (option.GetIndex() == obj)
					return yyjson_mut_strcpy(doc, option.GetEnglishName());
			return yyjson_mut_sint(doc, obj);
		}

		//template<typename T, typename O>
		//result_type visit_flags(T& obj, const FlagsInfo<O>& info) {
		//	return visit_primitive(obj, PrimitiveInfo<T>{});
		//}

		template<typename T, typename F>
		result_type visit_array(T arr, F f) {
			yyjson_mut_val* jarr = yyjson_mut_arr(doc);
			for (auto& obj : arr)
				yyjson_mut_arr_add_val(jarr, f(obj).value);
			return jarr;
		}

		template<typename T, typename F>
		result_type visit_tarray(T arr, F f) {
			yyjson_mut_val* jarr = yyjson_mut_arr(doc);
			for (auto& obj : arr)
				yyjson_mut_arr_add_val(jarr, f(obj).value);
			return jarr;
		}

		template<ucsl::reflection::accessors::CArrayAccessor T, typename F>
		result_type visit_carray(T obj, F f) {
			yyjson_mut_val* jarr = yyjson_mut_arr(doc);
			for (size_t i = 0; i < obj.get_length(); i++) {
				auto item = obj[i];
				yyjson_mut_arr_add_val(jarr, f(item).value);
			}
			return jarr;
		}

		template<ucsl::reflection::accessors::PointerAccessor T, typename F>
		result_type visit_pointer(T obj, F f) {
			if (!obj.get().has_value())
				return yyjson_mut_null(doc);

			auto v = obj.get().value();

			return f(v);
		}

		template<typename T, typename F>
		result_type visit_union(T obj, F f) {
			return f(obj);
		}

		template<typename T, typename F>
		result_type visit_type(T obj, F f) {
			return f(obj);
		}

		template<ucsl::reflection::accessors::ValueAccessor T, typename F>
		result_type visit_field(T obj, const traversals::FieldInfo& info, F f) {
			if (!info.erased)
				yyjson_mut_obj_add_val(doc, currentStruct, info.name, f(obj).value);
			return nullptr;
		}

		template<ucsl::reflection::accessors::StructureAccessor T, typename F>
		result_type visit_base_struct(T obj, F f) {
			return f(obj);
		}

		template<ucsl::reflection::accessors::StructureAccessor T, typename F>
		result_type visit_struct(T obj, F f) {
			yyjson_mut_val* thisStruct = yyjson_mut_obj(doc);
			yyjson_mut_val* prevStruct = currentStruct;
			currentStruct = thisStruct;
			result_type res = f(obj);
			currentStruct = prevStruct;

			return thisStruct;
		}

		template<ucsl::reflection::accessors::ValueAccessor T, typename F>
		result_type visit_root(T obj, F f) {
			return f(obj);
		}
	};

	//public:
	//	JsonSerializer(yyjson_mut_doc* doc) : doc{ doc } {
	//	}
	//	~JsonSerializer() {
	//	}

	//	template<typename T, typename R>
	//	yyjson_mut_val* serialize(T& data, R refl) {
	//		return ucsl::reflection::traversals::traversal<SerializeChunk>{ *this }(data, refl).value;
	//	}
	//};
}