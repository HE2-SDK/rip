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

		struct DisambiguatedPointer {
			std::optional<std::string> resolvedTarget{};
			std::vector<yyjson_mut_val*> weakPtrs{};
		};

		std::unordered_map<size_t, DisambiguatedPointer> knownPtrs{};
		std::vector<std::variant<const char*, size_t>> currentJsonPtr{};

		template<typename Refl>
		constexpr size_t countPtrDepth(const Refl& refl) {
			return refl.visit([&](auto r) {
				if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::POINTER) { return 1 + countPtrDepth(r.get_target_type()); }
				else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::CARRAY) { return 1 + countPtrDepth(r.get_item_type()); }
				else return 0;
			});
		}

		template<typename T>
		DisambiguatedPointer& lookupPtr(const T& obj) {
			return knownPtrs[static_cast<size_t>(&obj) ^ countPtrDepth(obj.refl)];
		}

		void resolvePtr(DisambiguatedPointer& knownPtr) {
			char buf[1024];

			size_t off{};

			for (const auto& el : currentJsonPtr)
				std::visit([&](const auto& val) {
					if constexpr (std::is_same_v<std::decay_t<decltype(val)>, const char*>)
						off += snprintf(buf + off, sizeof(buf) - off, "/%s", val);
					else
						off += snprintf(buf + off, sizeof(buf) - off, "/%zd", val);
				}, el);

			for (auto* weakPtr : knownPtr.weakPtrs)
				yyjson_mut_obj_add_strcpy(doc, weakPtr, "$ref", buf);

			knownPtr.weakPtrs.clear();
			knownPtr.resolvedTarget = buf;
		}

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

		inline yyjson_mut_val* process_primitive_data(const char* const& obj) {
			return yyjson_mut_strcpy(doc, obj);
		}

		template<ucsl::reflection::accessors::PrimitiveAccessor T>
		inline yyjson_mut_val* process_primitive(const T& obj) {
			return obj.visit([&](const auto& data) {
				if constexpr (std::is_same_v<typename decltype(data.refl)::repr, const char*> || std::is_same_v<typename decltype(data.refl)::repr, ucsl::strings::VariableString>)
					return process_primitive_data(std::string{ data }.c_str());
				else
					return process_primitive_data(static_cast<typename decltype(data.refl)::repr>(data));
			});
		}

		template<typename T>
		inline yyjson_mut_val* process_enum(const T& obj) {
			for (auto& option : obj.refl.get_options())
				if (option.GetIndex() == obj)
					return yyjson_mut_strcpy(doc, option.GetEnglishName());
			return yyjson_mut_sint(doc, obj);
		}

		inline yyjson_mut_val* process_bitfield(const auto& obj) {
			return obj.visit([&](const auto& underlying) {
				typename decltype(underlying.refl)::repr bitfield{ underlying };

				yyjson_mut_val* result = yyjson_mut_obj(doc);
				
				obj.refl.visit_components([&](const auto& component) {
					size_t mask{ (1 << component.get_width()) - 1 };
					size_t value{ (bitfield >> component.get_offset()) & mask };

					yyjson_mut_obj_add_val(doc, result, component.get_name(), component.get_type().visit([&](const auto& r) -> yyjson_mut_val* {
						if constexpr (std::decay_t<decltype(r)>::kind == ucsl::reflection::providers::TypeKind::PRIMITIVE)
							return r.visit([&](const auto& pd) -> yyjson_mut_val* {
								return process_primitive_data(*reinterpret_cast<typename std::decay_t<decltype(pd)>::repr*>(&value));
							});
						else if constexpr (std::decay_t<decltype(r)>::kind == ucsl::reflection::providers::TypeKind::ENUM) {
							for (auto& option : r.get_options())
								if (option.GetIndex() == value)
									return yyjson_mut_strcpy(doc, option.GetEnglishName());
							return yyjson_mut_sint(doc, value);
						}
						else static_assert(false, "invalid type kind");
					}));
				});

				return result;
			});
		}

		template<typename T>
		inline yyjson_mut_val* process_array(T arr) {
			yyjson_mut_val* jarr = yyjson_mut_arr(doc);
			size_t idx{};
			for (auto& obj : arr) {
				currentJsonPtr.push_back(idx++);
				yyjson_mut_arr_add_val(jarr, process_type(obj));
				currentJsonPtr.pop_back();
			}
			return jarr;
		}

		template<typename T>
		inline yyjson_mut_val* process_tarray(T arr) {
			yyjson_mut_val* jarr = yyjson_mut_arr(doc);
			size_t idx{};
			for (auto& obj : arr) {
				currentJsonPtr.push_back(idx++);
				yyjson_mut_arr_add_val(jarr, process_type(obj));
				currentJsonPtr.pop_back();
			}
			return jarr;
		}

		template<ucsl::reflection::accessors::CArrayAccessor T>
		inline yyjson_mut_val* process_carray(const T& arr) {
			yyjson_mut_val* jarr = yyjson_mut_arr(doc);
			size_t idx{};
			for (const auto& obj : arr) {
				currentJsonPtr.push_back(idx++);
				yyjson_mut_arr_add_val(jarr, process_type(obj));
				currentJsonPtr.pop_back();
			}
			return jarr;
		}

		template<ucsl::reflection::accessors::PointerAccessor T>
		inline yyjson_mut_val* process_pointer(const T& obj) {
			if (obj == nullptr)
				return yyjson_mut_null(doc);

			auto target = *obj;
			auto& knownPtr = lookupPtr(target);

			if (knownPtr.resolvedTarget.has_value()) {
				auto* obj = yyjson_mut_obj(doc);
				yyjson_mut_obj_add_strcpy(doc, obj, "$ref", knownPtr.resolvedTarget.value().c_str());
				return obj;
			}

			if constexpr (obj.refl.is_weak()) {
				auto* obj = yyjson_mut_obj(doc);
				knownPtr.weakPtrs.push_back(obj);
				return obj;
			}
			else {
				resolvePtr(knownPtr);

				target.visit([&](auto v) {
					if constexpr (decltype(v.refl)::kind == providers::TypeKind::CARRAY) {
						size_t idx{};

						for (const auto& item : v) {
							currentJsonPtr.push_back(idx++);
							resolvePtr(lookupPtr(item));
							currentJsonPtr.pop_back();
						}
					}
				});

				return process_type(target);
			}
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
				currentJsonPtr.push_back(fieldRefl.get_name());
				yyjson_mut_obj_add_val(doc, currentStruct, fieldRefl.get_name(), process_type(field));
				currentJsonPtr.pop_back();
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
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::BITFIELD) return process_bitfield(v);
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
