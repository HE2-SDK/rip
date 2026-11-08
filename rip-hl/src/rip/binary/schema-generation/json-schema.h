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

	class JsonSchemaGenerator {
		yyjson_mut_doc* doc;
		yyjson_mut_val* currentStruct{};

		template<std::integral T>
		inline yyjson_mut_val* process_primitive_data(const T& obj) {
			auto* res = yyjson_mut_obj(doc);

			yyjson_mut_obj_add_str(doc, res, "type", "integer");
			if (yyjson_mut_obj_add_int(doc, res, "minimum", )

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
			for (size_t i = 0; i < obj.matrix().rows(); i++)
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

		inline yyjson_mut_val* process_primitive_data(const char* const& obj) {
			return yyjson_mut_strcpy(doc, obj);
		}

		inline yyjson_mut_val* process_primitive(const auto& refl) {
			return refl.visit([&](const auto& data) {
				if constexpr (
					std::is_same_v<typename decltype(data)::repr, const char*>
					|| std::is_same_v<typename decltype(data)::repr, ucsl::strings::VariableString>
					|| (std::is_array_v<typename decltype(data)::repr> && std::is_same_v<std::remove_extent_t<typename decltype(data)::repr>, char>)
					) {
					auto* res = yyjson_mut_obj(doc);

					yyjson_mut_obj_add_str(doc, res, "type", "string");

					if constexpr (std::is_array_v<typename decltype(data)::repr> && std::is_same_v<std::remove_extent_t<typename decltype(data)::repr>, char>)
						yyjson_mut_obj_add_uint(doc, res, "maxLength", std::extent_v<typename decltype(data)::repr> - 1);

					return res;
				}
				else if constexpr (std::is_integral_v<typename decltype(data)::repr>) {
					auto* res = yyjson_mut_obj(doc);

					yyjson_mut_obj_add_str(doc, res, "type", "integer");

					if (data.range != nullptr) {
						yyjson_mut_obj_add_int(doc, res, "minimum", data.range.min);
						yyjson_mut_obj_add_int(doc, res, "maximum", data.range.max);
					}

					return res;
				}
				else if constexpr (std::is_same_v<typename decltype(data)::repr, float>) {
					auto* res = yyjson_mut_obj(doc);

					yyjson_mut_obj_add_str(doc, res, "type", "number");

					if (data.range != nullptr) {
						yyjson_mut_obj_add_float(doc, res, "minimum", data.range.min);
						yyjson_mut_obj_add_float(doc, res, "maximum", data.range.max);
					}

					return res;
				}
				else if constexpr (std::is_same_v<typename decltype(data)::repr, double>) {
					auto* res = yyjson_mut_obj(doc);

					yyjson_mut_obj_add_str(doc, res, "type", "number");

					if (data.range != nullptr) {
						yyjson_mut_obj_add_real(doc, res, "minimum", data.range.min);
						yyjson_mut_obj_add_real(doc, res, "maximum", data.range.max);
					}

					return res;
				}
				else {
					static_assert(false, "unknown primitive type");
					return yyjson_mut_obj(doc);
				}
			});
		}

		inline yyjson_mut_val* process_enum(const auto& refl) {
			auto* res = yyjson_mut_obj(doc);

			yyjson_mut_obj_add_str(doc, res, "type", "string");

			auto* opts = yyjson_mut_obj_add_arr(doc, res, "enum");

			for (auto& option : refl.get_options())
				yyjson_mut_arr_add_str(doc, opts, option.GetEnglishName());

			return res;
		}

		inline yyjson_mut_val* process_bitfield(const auto& refl) {
			yyjson_mut_val* res = yyjson_mut_obj(doc);

			yyjson_mut_obj_add_str(doc, res, "type", "object");

			yyjson_mut_val* required = yyjson_mut_obj_add_arr(doc, res, "required");
			yyjson_mut_val* props = yyjson_mut_obj_add_obj(doc, res, "properties");
				
			refl.visit_components([&](const auto& component) {
				yyjson_mut_arr_add_str(doc, required, component.get_name());
				yyjson_mut_obj_add_val(doc, props, component.get_name(), component.get_type().visit([&](const auto& r) -> yyjson_mut_val* {
					if constexpr (std::decay_t<decltype(r)>::kind == ucsl::reflection::providers::TypeKind::PRIMITIVE) return process_primitive(r);
					else if constexpr (std::decay_t<decltype(r)>::kind == ucsl::reflection::providers::TypeKind::ENUM) return process_enum(r);
					else static_assert(false, "invalid type kind");
				}));
			});

			return res;
		}

		inline yyjson_mut_val* process_array(const auto& refl) {
			yyjson_mut_val* res = yyjson_mut_obj(doc);

			yyjson_mut_obj_add_str(doc, res, "type", "array");
			yyjson_mut_obj_add_val(doc, res, "items", process_type(refl.get_item_type()));

			return res;
		}

		inline yyjson_mut_val* process_tarray(const auto& refl) {
			yyjson_mut_val* res = yyjson_mut_obj(doc);

			yyjson_mut_obj_add_str(doc, res, "type", "array");
			yyjson_mut_obj_add_val(doc, res, "items", process_type(refl.get_item_type()));

			return res;
		}

		inline yyjson_mut_val* process_carray(const auto& refl) {
			yyjson_mut_val* res = yyjson_mut_obj(doc);

			yyjson_mut_obj_add_str(doc, res, "type", "array");
			yyjson_mut_obj_add_val(doc, res, "items", process_type(refl.get_item_type()));

			return res;
		}

		inline yyjson_mut_val* process_pointer(const auto& refl) {
			yyjson_mut_val* res = yyjson_mut_obj(doc);
			yyjson_mut_val* opts = yyjson_mut_obj_add_arr(doc, res, "oneOf");

			yyjson_mut_val* nullOpt = yyjson_mut_arr_add_obj(doc, opts);
			yyjson_mut_obj_add_str(doc, nullOpt, "type", "null");

			yyjson_mut_val* refOpt = yyjson_mut_arr_add_obj(doc, opts);
			yyjson_mut_obj_add_str(doc, refOpt, "type", "object");
			yyjson_mut_val* refRequired = yyjson_mut_obj_add_arr(doc, refOpt, "required");
			yyjson_mut_val* refProps = yyjson_mut_obj_add_obj(doc, refOpt, "properties");
			yyjson_mut_arr_add_str(doc, refRequired, "$ref");
			yyjson_mut_val* refRefObj = yyjson_mut_obj_add_obj(doc, refProps, "$ref");
			yyjson_mut_obj_add_str(doc, refRefObj, "type", "string");

			if constexpr (!obj.refl.is_weak())
				yyjson_mut_arr_add_val(opts, process_type(refl.get_target_type()));

			return res;
		}

		inline yyjson_mut_val* process_union(const auto& refl) {
			yyjson_mut_val* res = yyjson_mut_obj(doc);
			yyjson_mut_val* opts = yyjson_mut_obj_add_arr(doc, res, "oneOf");

			refl.visit([&](const auto& field) { yyjson_mut_arr_add_val(opts, process_type(field)); });

			return res;
		}

		inline void process_fields(yyjson_mut_val* obj, const auto& refl) {
			auto base = refl.get_base();

			if (base.has_value())
				process_fields(obj, base.value());

			refl.visit_fields([&](const auto& fieldRefl) {
				yyjson_mut_obj_add_val(doc, obj, fieldRefl.get_name(), process_type(fieldRefl));
			});
		}

		inline yyjson_mut_val* process_struct(const auto& refl) {
			yyjson_mut_val* res = yyjson_mut_obj(doc);
			process_fields(res, refl);
			return res;
		}

		inline yyjson_mut_val* process_type(const auto& refl) {
			return refl.visit([&](auto v) {
				if constexpr (decltype(v)::kind == providers::TypeKind::PRIMITIVE) return process_primitive(v);
				else if constexpr (decltype(v)::kind == providers::TypeKind::ENUM) return process_enum(v);
				else if constexpr (decltype(v)::kind == providers::TypeKind::BITFIELD) return process_bitfield(v);
				else if constexpr (decltype(v)::kind == providers::TypeKind::ARRAY) return process_array(v);
				else if constexpr (decltype(v)::kind == providers::TypeKind::TARRAY) return process_tarray(v);
				else if constexpr (decltype(v)::kind == providers::TypeKind::CARRAY) return process_carray(v);
				else if constexpr (decltype(v)::kind == providers::TypeKind::POINTER) return process_pointer(v);
				else if constexpr (decltype(v)::kind == providers::TypeKind::UNION) return process_union(v);
				else if constexpr (decltype(v)::kind == providers::TypeKind::STRUCTURE) return process_struct(v);
				else static_assert(false, "invalid type kind");
			});
		}

	public:
		JsonSchemaGenerator(yyjson_mut_doc* doc) : doc{ doc } {}

		inline yyjson_mut_val* process(const auto& refl) {
			return process_type(obj);
		}
	};

	inline yyjson_mut_val* generateJsonSchema(yyjson_mut_doc* doc, const auto& refl) {
		JsonSchemaGenerator generator{ doc };
		return generator.process(refl);
	}
}
