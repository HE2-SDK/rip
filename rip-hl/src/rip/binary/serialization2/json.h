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
	class JsonSerializer {
		yyjson_mut_doc* doc;
		yyjson_mut_val* currentStruct{};

		template<std::integral T, std::enable_if_t<std::is_signed_v<T>, bool> = true>
		inline yyjson_mut_val* process_primitive_data(const T& obj) {
			return yyjson_mut_sint(doc, obj);
		}

		template<std::integral T, std::enable_if_t<!std::is_signed_v<T>, bool> = true>
		inline yyjson_mut_val* process_primitive_data(const T& obj) {
			return yyjson_mut_uint(doc, obj);
		}

		inline yyjson_mut_val* process_primitive_data(const float& obj) {
			return yyjson_mut_float(doc, obj);
		}

		inline yyjson_mut_val* process_primitive_data(const double& obj) {
			return yyjson_mut_real(doc, obj);
		}

		inline yyjson_mut_val* process_primitive_data(const bool& obj) {
			return yyjson_mut_bool(doc, obj);
		}

		inline yyjson_mut_val* process_primitive_data(const ucsl::math::Vector2& obj) {
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

		inline yyjson_mut_val* process_primitive_data(const ucsl::math::Vector3& obj) {
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

		inline yyjson_mut_val* process_primitive_data(const ucsl::math::Position& obj) {
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

		inline yyjson_mut_val* process_primitive_data(const ucsl::math::Vector4& obj) {
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

		inline yyjson_mut_val* process_primitive_data(const ucsl::math::Quaternion& obj) {
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

		inline yyjson_mut_val* process_primitive_data(const ucsl::math::Matrix34& obj) {
			yyjson_mut_val* res = yyjson_mut_arr(doc);
			for (size_t i = 0; i < obj.rows(); i++)
				for (size_t j = 0; j < obj.cols(); j++)
					yyjson_mut_arr_add_float(doc, res, obj(i, j));
			return res;
		}

		inline yyjson_mut_val* process_primitive_data(const ucsl::math::Matrix44& obj) {
			yyjson_mut_val* res = yyjson_mut_arr(doc);
			for (size_t i = 0; i < obj.rows(); i++)
				for (size_t j = 0; j < obj.cols(); j++)
					yyjson_mut_arr_add_float(doc, res, obj(i, j));
			return res;
		}

		template<ucsl::colors::ChannelOrder order>
		inline yyjson_mut_val* process_primitive_data(const ucsl::colors::Color8<order>& obj) {
			yyjson_mut_val* res = yyjson_mut_obj(doc);
			yyjson_mut_obj_add_uint(doc, res, "r", obj.r);
			yyjson_mut_obj_add_uint(doc, res, "g", obj.g);
			yyjson_mut_obj_add_uint(doc, res, "b", obj.b);
			yyjson_mut_obj_add_uint(doc, res, "a", obj.a);
			return res;
		}

		template<ucsl::colors::ChannelOrder order>
		inline yyjson_mut_val* process_primitive_data(const ucsl::colors::Colorf<order>& obj) {
			yyjson_mut_val* res = yyjson_mut_obj(doc);
			yyjson_mut_obj_add_float(doc, res, "r", obj.r);
			yyjson_mut_obj_add_float(doc, res, "g", obj.g);
			yyjson_mut_obj_add_float(doc, res, "b", obj.b);
			yyjson_mut_obj_add_float(doc, res, "a", obj.a);
			return res;
		}

		inline yyjson_mut_val* process_primitive_data(const ucsl::objectids::ObjectIdV1& obj) {
			char guid[39];
			util::toGUID(obj, guid);
			return yyjson_mut_strcpy(doc, guid);
		}

		inline yyjson_mut_val* process_primitive_data(const ucsl::objectids::ObjectIdV2& obj) {
			char guid[39];
			util::toGUID(obj, guid);
			return yyjson_mut_strcpy(doc, guid);
		}

		inline yyjson_mut_val* process_primitive_data(const ucsl::strings::VariableString& obj) {
			return yyjson_mut_strcpy(doc, obj.c_str());
		}

		inline yyjson_mut_val* process_primitive_data(const char* const& obj) {
			return yyjson_mut_strcpy(doc, obj);
		}

		inline yyjson_mut_val* process_primitive_data(void* const& obj) {
			return yyjson_mut_str(doc, "TODO");
		}

		template<ucsl::reflection::accessors::PrimitiveAccessor T>
		inline yyjson_mut_val* process_primitive(const T& obj) {
			return obj.visit([&](auto data) {
				if constexpr (std::is_same_v<typename decltype(data.refl)::repr, const char*>)
					return process_primitive_data(std::string{ data }.c_str());
				else
					return process_primitive_data(typename decltype(data.refl)::repr{ data });
			});
		}

		template<typename T>
		inline yyjson_mut_val* process_enum(const T& obj) {
			for (auto& option : obj.refl.get_options())
				if (option.GetIndex() == obj)
					return yyjson_mut_strcpy(doc, option.GetEnglishName());
			return yyjson_mut_sint(doc, obj);
		}

		//template<typename T, typename O>
		//yyjson_mut_val* visit_flags(T& obj, const FlagsInfo<O>& info) {
		//	return visit_primitive(obj, PrimitiveInfo<T>{});
		//}

		template<typename T>
		inline yyjson_mut_val* process_array(T arr) {
			yyjson_mut_val* jarr = yyjson_mut_arr(doc);
			for (auto& obj : arr)
				yyjson_mut_arr_add_val(jarr, process_type(obj));
			return jarr;
		}

		template<typename T>
		inline yyjson_mut_val* process_tarray(T arr) {
			yyjson_mut_val* jarr = yyjson_mut_arr(doc);
			for (auto& obj : arr)
				yyjson_mut_arr_add_val(jarr, process_type(obj));
			return jarr;
		}

		template<ucsl::reflection::accessors::CArrayAccessor T>
		inline yyjson_mut_val* process_carray(const T& arr) {
			yyjson_mut_val* jarr = yyjson_mut_arr(doc);
			for (const auto& obj : arr)
				yyjson_mut_arr_add_val(jarr, process_type(obj));
			return jarr;
		}

		template<ucsl::reflection::accessors::PointerAccessor T>
		inline yyjson_mut_val* process_pointer(const T& obj) {
			if (!obj.get().has_value())
				return yyjson_mut_null(doc);

			auto v = obj.get().value();

			return process_type(v);
		}

		template<typename T>
		inline yyjson_mut_val* process_union(const T& obj) {
			yyjson_mut_val* res{};
			obj.visit([&](auto field) { res = process_type(field); });
			return res;
		}

		template<ucsl::reflection::accessors::StructureAccessor T>
		inline yyjson_mut_val* process_fields(const T& obj) {
			auto base = obj.get_base();

			if (base.has_value())
				process_fields(base.value());

			obj.visit_fields([&](const auto& field, const auto& fieldRefl) {
				yyjson_mut_obj_add_val(doc, currentStruct, fieldRefl.get_name(), process_type(field));
			});

			return nullptr;
		}

		template<ucsl::reflection::accessors::StructureAccessor T>
		inline yyjson_mut_val* process_struct(const T& obj) {
			yyjson_mut_val* thisStruct = yyjson_mut_obj(doc);
			yyjson_mut_val* prevStruct = currentStruct;
			currentStruct = thisStruct;
			process_fields(obj);
			currentStruct = prevStruct;
			return thisStruct;
		}

		template<ucsl::reflection::accessors::ValueAccessor T>
		inline yyjson_mut_val* process_type(const T& obj) {
			return obj.visit([&](auto v) {
				if constexpr (decltype(v.refl)::kind == providers::TypeKind::PRIMITIVE) return process_primitive(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::ENUM) return process_enum(v);
				//else if constexpr (decltype(v.refl)::kind == providers::TypeKind::FLAGS) return process_flags(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::ARRAY) return process_array(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::TARRAY) return process_tarray(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::CARRAY) return process_carray(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::POINTER) return process_pointer(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::UNION) return process_union(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::STRUCTURE) return process_struct(v);
				else static_assert(false, "invalid type kind");
			});
		}

	public:
		JsonSerializer(yyjson_mut_doc* doc) : doc{ doc } {}

		template<ucsl::reflection::accessors::ValueAccessor T>
		inline yyjson_mut_val* process(const T& obj) {
			return process_type(obj);
		}
	};

	template<bool arrayVectors = false>
	inline yyjson_mut_val* serializeJson(yyjson_mut_doc* doc, const auto& acc) {
		JsonSerializer<arrayVectors> serializer{ doc };
		return serializer.process(acc);
	}
}
