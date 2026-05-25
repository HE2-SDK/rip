#pragma once
#include <string>
#include <iostream>
#include <ucsl/containers/arrays/array.h>
#include <ucsl/containers/arrays/tarray.h>
#include <ucsl/math.h>
#include <ucsl/colors.h>
#include <ucsl/strings/variable-string.h>
#include <ucsl/rfl/ranges.h>
#include <ucsl-reflection/game-interfaces/standalone/reflection-db.h>
#include <ucsl-reflection/game-interfaces/standalone/rflsystem.h>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <rip/util/memory.h>

namespace rip::schemas::hedgeset {
	using namespace ucsl::rfl::ranges;

	namespace json_reflections {
		struct EnumValueDef {
			int value{};
			std::optional<rfl::Object<std::string>> descriptions{};
		};

		struct EnumDef {
			std::string type{};
			rfl::Object<EnumValueDef> values{};
		};

		template<typename T> struct rfl_range_rep {};
		template<> struct rfl_range_rep<RangeSint32> { using type = int32_t; };
		template<> struct rfl_range_rep<RangeUint32> { using type = uint32_t; };
		template<> struct rfl_range_rep<RangeSint64> { using type = int64_t; };
		template<> struct rfl_range_rep<RangeUint64> { using type = uint64_t; };
		template<> struct rfl_range_rep<RangeFloat> { using type = float; };
		template<> struct rfl_range_rep<RangeVector2> { using type = std::array<float, 2>; };
		template<> struct rfl_range_rep<RangeVector3> { using type = std::array<float, 3>; };
		template<> struct rfl_range_rep<RangeVector4> { using type = std::array<float, 4>; };

		template<typename T>
		using rfl_range_rep_t = typename rfl_range_rep<T>::type;

		using Ranges = rfl::Variant<
			rfl_range_rep_t<RangeSint32>,
			rfl_range_rep_t<RangeUint32>,
			rfl_range_rep_t<RangeSint64>,
			rfl_range_rep_t<RangeUint64>,
			rfl_range_rep_t<RangeFloat>,
			rfl_range_rep_t<RangeVector2>,
			rfl_range_rep_t<RangeVector3>,
			rfl_range_rep_t<RangeVector4>
		>;

		struct RangeProps {
			rfl::SkipDeserialization<std::optional<Ranges>> min_range{};
			rfl::SkipDeserialization<std::optional<Ranges>> max_range{};
			rfl::SkipDeserialization<std::optional<Ranges>> step{};
		};

		struct MemberDef {
			std::string name{};
			std::string type{};
			std::optional<std::string> subtype{};
			std::optional<rfl::Object<EnumValueDef>> flags{};
			std::optional<unsigned int> array_size{};
			std::optional<unsigned int> alignment{};
			rfl::Flatten<RangeProps> range;
			std::optional<rfl::Object<std::string>> descriptions{};
		};

		struct StructDef {
			std::optional<std::string> parent{ std::nullopt };
			std::optional<std::vector<MemberDef>> fields{};
		};

		struct ObjectDef {
			rfl::Rename<"struct", std::optional<std::string>> structName{};
			std::optional<std::string> category{};
		};

		struct TagDef {
			rfl::Rename<"struct", std::optional<std::string>> structName{};
		};

		struct Template {
			unsigned int version{};
			std::string format{};
			std::map<std::string, EnumDef> enums{};
			std::map<std::string, StructDef> structs{};
			std::map<std::string, ObjectDef> objects{};
			std::map<std::string, TagDef> tags{};
		};
	}

	using Template = json_reflections::Template;

	enum class HSONFormat {
		GEDIT_V2,
		GEDIT_V3,
		SOBJ_V1
	};

	using namespace ucsl::reflection::game_interfaces::standalone;

	class schema_builder {
		using MemberType = StandaloneRflSystem::RflClassMember::Type;

		const Template& templ;
		Schema schema{};

		struct LoadTypeConversionResult {
			MemberType type{ MemberType::VOID };
			MemberType subtype{ MemberType::VOID };
			std::optional<std::shared_ptr<StandaloneRflSystem::RflClass>> structt{ std::nullopt };
			std::optional<std::shared_ptr<StandaloneRflSystem::RflClassEnum>> enumm{ std::nullopt };
			std::optional<std::vector<StandaloneRflSystem::RflClassEnumMember>> flag_values{ std::nullopt };
		};

		MemberType get_primitive_type(const std::string& type, const std::optional<unsigned int>& alignment = std::nullopt) {
			if (type == "int8") return MemberType::SINT8;
			if (type == "uint8") return MemberType::UINT8;
			if (type == "int16") return MemberType::SINT16;
			if (type == "uint16") return MemberType::UINT16;
			if (type == "int32") return MemberType::SINT32;
			if (type == "uint32") return MemberType::UINT32;
			if (type == "int64") return MemberType::SINT64;
			if (type == "uint64") return MemberType::UINT64;
			if (type == "bool") return MemberType::BOOL;
			if (type == "float32") return MemberType::FLOAT;
			if (type == "vector2") return MemberType::VECTOR2;
			if (type == "vector3") return alignment.has_value() && alignment.value() == 16 ? MemberType::VECTOR3 : MemberType::POSITION;
			if (type == "vector4") return MemberType::VECTOR4;
			if (type == "quaternion") return MemberType::QUATERNION;
			if (type == "matrix34") return MemberType::MATRIX34;
			if (type == "matrix44") return MemberType::MATRIX44;
			if (type == "color8") return templ.format == "gedit_v2" ? MemberType::COLOR_BYTE_RGBA : MemberType::COLOR_BYTE_ABGR;
			if (type == "colorf") return templ.format == "gedit_v2" ? MemberType::COLOR_FLOAT_RGBA : MemberType::COLOR_FLOAT_ABGR;
			if (type == "string") return MemberType::STRING;
			if (type == "object_reference") return templ.format == "gedit_v2" ? MemberType::OBJECT_ID_V1 : MemberType::OBJECT_ID_V2;
			return MemberType::VOID;
		}

		static std::vector<StandaloneRflSystem::RflClassEnumMember> get_flag_values(const json_reflections::MemberDef& member) {
			std::vector<StandaloneRflSystem::RflClassEnumMember> flag_values{};

			for (auto& [name, enumValueDef] : member.flags.value())
				flag_values.push_back(StandaloneRflSystem::RflClassEnumMember{ enumValueDef.value, name, enumValueDef.descriptions.has_value() ? enumValueDef.descriptions.value().get("ja").value_or("") : "" });

			return flag_values;
		}

		LoadTypeConversionResult get_type(const json_reflections::MemberDef& member, std::map<std::string, std::shared_ptr<StandaloneRflSystem::RflClassEnum>>& enums) {
			if (templ.structs.contains(member.type))
				return { .type = MemberType::STRUCT, .structt = load_rfl_class(member.type) };

			if (templ.enums.contains(member.type)) {
				auto& e = templ.enums.at(member.type);

				return { .type = MemberType::ENUM, .subtype = get_primitive_type(e.type), .enumm = load_enum(member.type, enums) };
			}

			if (member.flags.has_value())
				return { .type = MemberType::FLAGS, .subtype = get_primitive_type(member.type), .flag_values = get_flag_values(member) };

			if (member.type != "array")
				return { .type = get_primitive_type(member.type, member.alignment) };

			auto& subtype = member.subtype.value();

			if (member.array_size.has_value() && member.array_size.value() > 0) {
				if (templ.structs.contains(subtype))
					return { .type = MemberType::STRUCT, .structt = load_rfl_class(subtype) };

				if (templ.enums.contains(subtype)) {
					auto& e = templ.enums.at(subtype);

					return { .type = MemberType::ENUM, .subtype = get_primitive_type(e.type), .enumm = load_enum(subtype, enums) };
				}

				return { .type = get_primitive_type(subtype, member.alignment) };
			}

			if (templ.format != "gedit_v3") {
				if (templ.structs.contains(subtype))
					return { .type = MemberType::OLD_ARRAY, .subtype = MemberType::STRUCT, .structt = load_rfl_class(subtype) };

				return { .type = MemberType::OLD_ARRAY, .subtype = get_primitive_type(subtype, member.alignment) };
			}

			if (templ.structs.contains(subtype))
				return { .type = MemberType::ARRAY, .subtype = MemberType::STRUCT, .structt = load_rfl_class(subtype) };

			return { .type = MemberType::ARRAY, .subtype = get_primitive_type(subtype, member.alignment) };
		}

		std::shared_ptr<StandaloneRflSystem::RflClassEnum> load_enum(const std::string& name, std::map<std::string, std::shared_ptr<StandaloneRflSystem::RflClassEnum>>& enums) {
			const json_reflections::EnumDef& enumDef = templ.enums.at(name);

			auto [resIt, resSuccess] = enums.emplace(name, std::make_shared<StandaloneRflSystem::RflClassEnum>(name, std::vector<typename StandaloneRflSystem::RflClassEnumMember>{}));
			auto [resName, res] = *resIt;

			std::vector<typename StandaloneRflSystem::RflClassEnumMember> enumMembers{};

			for (auto& [enumValueName, enumValueDef] : enumDef.values)
				enumMembers.push_back(typename StandaloneRflSystem::RflClassEnumMember{ enumValueDef.value, enumValueName, enumValueDef.descriptions.has_value() ? enumValueDef.descriptions.value().get("ja").value_or("") : "" });

			res->values = std::move(enumMembers);

			return enums[name];
		}

		std::shared_ptr<StandaloneRflSystem::RflClass> load_rfl_class(const std::string& name) {
			if (schema.classes.contains(name))
				return schema.classes[name];

			const json_reflections::StructDef& structDef = templ.structs.at(name);

			auto [resIt, resSuccess] = schema.classes.emplace(name, std::make_shared<StandaloneRflSystem::RflClass>(name, std::nullopt, 0, std::vector<std::shared_ptr<StandaloneRflSystem::RflClassEnum>>{}, std::vector<std::shared_ptr<StandaloneRflSystem::RflClassMember>>{}, 0));
			auto [resName, res] = *resIt;

			std::map<std::string, std::shared_ptr<StandaloneRflSystem::RflClassEnum>> structEnums{};
			std::vector<std::shared_ptr<StandaloneRflSystem::RflClassMember>> structMembers{};

			unsigned int offset{};

			std::optional<std::shared_ptr<StandaloneRflSystem::RflClass>> parent{};

			if (structDef.parent.has_value()) {
				parent = load_rfl_class(structDef.parent.value());
				offset = parent.value()->GetSize();
			}

			res->parent = parent;

			if (structDef.fields.has_value()) {
				for (auto& memberDef : structDef.fields.value()) {
					auto convertedTypes = get_type(memberDef, structEnums);

					auto member = structMembers.emplace_back(std::make_shared<StandaloneRflSystem::RflClassMember>(
						memberDef.name,
						convertedTypes.structt,
						convertedTypes.enumm,
						std::move(convertedTypes.flag_values),
						convertedTypes.type,
						convertedTypes.subtype,
						(memberDef.array_size.has_value() && memberDef.array_size.value() > 0) ? memberDef.array_size.value() : 0,
						0
					));

					member->offset = offset = align(offset, member->GetAlignment());
					offset += (unsigned int)member->GetSize();
				}
			}

			auto vals = std::views::values(structEnums);
			std::ranges::copy(vals.begin(), vals.end(), std::back_inserter(res->enums));

			res->members = std::move(structMembers);
			res->size = align(offset, res->GetAlignment());

			return schema.classes[name];
		}

		Schema::ObjectInfo load_object(const std::string& name) {
			if (schema.objects.contains(name))
				return schema.objects[name];

			const json_reflections::ObjectDef& objectDef = templ.objects.at(name);

			auto [resIt, resSuccess] = schema.objects.emplace(name, Schema::ObjectInfo{ objectDef.structName.value().has_value() ? std::make_optional(load_rfl_class(objectDef.structName.value().value())) : std::nullopt, objectDef.category });

			return resIt->second;
		}

		Schema::ComponentInfo load_tag(const std::string& name) {
			if (schema.components.contains(name))
				return schema.components[name];

			const json_reflections::TagDef& tagDef = templ.tags.at(name);

			auto [resIt, resSuccess] = schema.components.emplace(name, Schema::ComponentInfo{ tagDef.structName.value().has_value() ? std::make_optional(load_rfl_class(tagDef.structName.value().value())) : std::nullopt });

			return resIt->second;
		}

	public:
		schema_builder(const Template& templ) : templ{ templ } {
			//for (auto& [name, enumDef] : templ.enums)
			//	load_enum(name, enumDef);

			for (auto& [name, structDef] : templ.structs)
				load_rfl_class(name);

			for (auto& [name, objectDef] : templ.objects)
				load_object(name);

			for (auto& [name, tagDef] : templ.tags)
				load_tag(name);
		}

		const Schema& get_schema() const {
			return schema;
		}
	};

	template<typename GameInterface>
	class template_builder {
	protected:
		json_reflections::Template templ{};

		struct PrimitiveTypeConversionResult {
			std::string type{};
			json_reflections::RangeProps range{};
			std::optional<unsigned int> alignment{ std::nullopt };
		};

		struct TypeConversionResult {
			std::string type{};
			std::optional<std::string> subtype{ std::nullopt };
			std::optional<unsigned int> alignment{ std::nullopt };
			std::optional<unsigned int> array_size{ std::nullopt };
			std::optional<rfl::Object<json_reflections::EnumValueDef>> flag_values{ std::nullopt };
			json_reflections::RangeProps range{};
		};

		using MemberType = typename GameInterface::RflSystem::RflClassMember::Type;

		bool is_arraylike(MemberType type) {
			if constexpr (GameInterface::RflSystem::TypeSet::supports_old_array)
				return type == MemberType::OLD_ARRAY || type == MemberType::ARRAY;
			else
				return type == MemberType::ARRAY;
		}

		template<typename T>
		bool is_useful_range_value(T range) {
			return range > std::numeric_limits<T>::lowest() / 2 && range < std::numeric_limits<T>::max() / 2;
		}

		template<typename Range> std::optional<json_reflections::rfl_range_rep_t<Range>> get_range_value(const range_value_t<Range>& range) { return { range }; }
		template<> std::optional<json_reflections::rfl_range_rep_t<RangeFloat>> get_range_value<RangeFloat>(const range_value_t<RangeFloat>& range) {
			return is_useful_range_value(range)
				? std::make_optional<json_reflections::rfl_range_rep_t<RangeFloat>>(range)
				: std::nullopt;
		}
		template<> std::optional<json_reflections::rfl_range_rep_t<RangeVector2>> get_range_value<RangeVector2>(const range_value_t<RangeVector2>& range) {
			return is_useful_range_value(range.x) && is_useful_range_value(range.y)
				? std::make_optional<json_reflections::rfl_range_rep_t<RangeVector2>>({ range.x, range.y })
				: std::nullopt;
		}
		template<> std::optional<json_reflections::rfl_range_rep_t<RangeVector3>> get_range_value<RangeVector3>(const range_value_t<RangeVector3>& range) {
			return is_useful_range_value(range.x) && is_useful_range_value(range.y) && is_useful_range_value(range.z)
				? std::make_optional<json_reflections::rfl_range_rep_t<RangeVector3>>({ range.x, range.y, range.z })
				: std::nullopt;
		}
		template<> std::optional<json_reflections::rfl_range_rep_t<RangeVector4>> get_range_value<RangeVector4>(const range_value_t<RangeVector4>& range) {
			return is_useful_range_value(range.x) && is_useful_range_value(range.y) && is_useful_range_value(range.z) && is_useful_range_value(range.w)
				? std::make_optional<json_reflections::rfl_range_rep_t<RangeVector4>>({ range.x, range.y, range.z, range.w })
				: std::nullopt;
		}

		template<typename Range>
		json_reflections::RangeProps generate_range(const typename GameInterface::RflSystem::RflClassMember& member) {
			auto* range = member.GetRange<Range>();

			return range == nullptr ? json_reflections::RangeProps{} : json_reflections::RangeProps{ get_range_value<Range>(range->min), get_range_value<Range>(range->max), get_range_value<Range>(range->step) };
		}

		PrimitiveTypeConversionResult get_primitive_type(const typename GameInterface::RflSystem::RflClassMember& member, MemberType type) {
			if (type == MemberType::SINT8) return { .type = "int8", .range = generate_range<RangeSint32>(member) };
			if (type == MemberType::UINT8) return { .type = "uint8", .range = generate_range<RangeUint32>(member) };
			if (type == MemberType::SINT16) return { .type = "int16", .range = generate_range<RangeSint32>(member) };
			if (type == MemberType::UINT16) return { .type = "uint16", .range = generate_range<RangeUint32>(member) };
			if (type == MemberType::SINT32) return { .type = "int32", .range = generate_range<RangeSint32>(member) };
			if (type == MemberType::UINT32) return { .type = "uint32", .range = generate_range<RangeUint32>(member) };
			if (type == MemberType::SINT64) return { .type = "int64", .range = generate_range<RangeSint64>(member) };
			if (type == MemberType::UINT64) return { .type = "uint64", .range = generate_range<RangeUint64>(member) };
			if (type == MemberType::BOOL) return { .type = "bool" };
			if (type == MemberType::FLOAT) return { .type = "float32", .range = generate_range<RangeFloat>(member) };
			if (type == MemberType::VECTOR2) return { .type = "vector2", .range = generate_range<RangeVector2>(member) };
			if (type == MemberType::VECTOR3) return { .type = "vector3", .range = generate_range<RangeVector3>(member), .alignment = 16 };
			if (type == MemberType::VECTOR4) return { .type = "vector4", .range = generate_range<RangeVector4>(member) };
			if (type == MemberType::POSITION) return { .type = "vector3", .range = generate_range<RangeVector3>(member) };
			if (type == MemberType::QUATERNION) return { .type = "quaternion" };
			if (type == MemberType::MATRIX34) return { .type = "matrix34" };
			if (type == MemberType::MATRIX44) return { .type = "matrix44" };
			if constexpr (GameInterface::RflSystem::TypeSet::template supports_primitive<ucsl::colors::Color8RGBA>)
				if (type == MemberType::COLOR_BYTE_RGBA) return { .type = "color8" };
			if constexpr (GameInterface::RflSystem::TypeSet::template supports_primitive<ucsl::colors::ColorfRGBA>)
				if (type == MemberType::COLOR_FLOAT_RGBA) return { .type = "colorf" };
			if constexpr (GameInterface::RflSystem::TypeSet::template supports_primitive<ucsl::colors::Color8ABGR>)
				if (type == MemberType::COLOR_BYTE_ABGR) return { .type = "color8" };
			if constexpr (GameInterface::RflSystem::TypeSet::template supports_primitive<ucsl::colors::ColorfABGR>)
				if (type == MemberType::COLOR_FLOAT_ABGR) return { .type = "colorf" };
			if (type == MemberType::STRING) return { .type = "string" };
			if constexpr (GameInterface::RflSystem::TypeSet::template supports_primitive<ucsl::objectids::ObjectIdV1>)
				if (type == MemberType::OBJECT_ID_V1) return { .type = "object_reference" };
			if constexpr (GameInterface::RflSystem::TypeSet::template supports_primitive<ucsl::objectids::ObjectIdV2>)
				if (type == MemberType::OBJECT_ID_V2) return { .type = "object_reference" };
			assert(false && "unknown type");
			return { .type = "unknown" };
		}

		TypeConversionResult get_type(const typename GameInterface::RflSystem::RflClassMember& member, const std::string& cur_namespace) {
			unsigned int array_size = member.GetArrayLength();
			MemberType type = member.GetType();
			MemberType subtype = member.GetSubType();

			if (array_size > 0) {
				assert(!is_arraylike(type) && "C arrays of arrays cannot be represented in HedgeSet templates");

				switch (type) {
				case MemberType::STRUCT: return { .type = "array", .subtype = get_struct(*member.GetClass()), .array_size = array_size };
				case MemberType::ENUM: return { .type = "array", .subtype = get_enum(*member.GetEnum(), cur_namespace, subtype), .array_size = array_size };
				case MemberType::FLAGS: return { .type = "array", .subtype = get_primitive_type(member, subtype).type, .array_size = array_size, .flag_values = member.GetFlagValues() ? std::make_optional(generate_enum_values(*member.GetFlagValues())) : std::nullopt };
				default: {
					auto prim = get_primitive_type(member, type);

					return { .type = "array", .subtype = prim.type, .alignment = prim.alignment, .array_size = array_size, .range = prim.range };
				}
				}
			}

			MemberType array_type{ MemberType::ARRAY };

			if constexpr (GameInterface::RflSystem::TypeSet::supports_old_array) {
				assert(type != MemberType::ARRAY && "type ARRAY cannot be represented in HedgeSet templates if the type system supports OLD_ARRAY");

				array_type = MemberType::OLD_ARRAY;
			}

			if (type == array_type) {
				switch (subtype) {
				case MemberType::STRUCT: return { .type = "array", .subtype = get_struct(*member.GetClass()) };
				default: {
					auto prim = get_primitive_type(member, subtype);

					return { .type = "array", .subtype = prim.type, .alignment = prim.alignment, .range = prim.range };
				}
				}
			}

			switch (type) {
			case MemberType::STRUCT: return { .type = get_struct(*member.GetClass()) };
			case MemberType::ENUM: return { .type = get_enum(*member.GetEnum(), cur_namespace, subtype) };
			case MemberType::FLAGS: return { .type = get_primitive_type(member, subtype).type, .flag_values = member.GetFlagValues() ? std::make_optional(generate_enum_values(*member.GetFlagValues())) : std::nullopt };
			default: {
				auto prim = get_primitive_type(member, type);

				return { .type = prim.type, .alignment = prim.alignment, .range = prim.range };
			}
			}
		}

		std::optional<rfl::Object<std::string>> generate_descs(const char* jaText) {
			if (jaText == nullptr)
				return std::nullopt;

			rfl::Object<std::string> descs{};
			descs["ja"] = jaText;
			return descs;
		}

		json_reflections::EnumValueDef generate_enum_value(const typename GameInterface::RflSystem::RflClassEnumMember& value) {
			return { .value = value.GetIndex(), .descriptions = generate_descs(value.GetJapaneseName()) };
		}

		template<typename R>
		rfl::Object<json_reflections::EnumValueDef> generate_enum_values(const R& values) {
			rfl::Object<json_reflections::EnumValueDef> res{};

			for (auto& value : values)
				res[value.GetEnglishName()] = generate_enum_value(value);

			return res;
		}

		json_reflections::EnumDef generate_enum(const GameInterface::RflSystem::RflClassEnum& enumClass, const std::string& name, MemberType type) {
			switch (type) {
			case MemberType::SINT8: return { .type = "int8", .values = generate_enum_values(enumClass.GetValues()) };
			case MemberType::UINT8: return { .type = "uint8", .values = generate_enum_values(enumClass.GetValues()) };
			case MemberType::SINT16: return { .type = "int16", .values = generate_enum_values(enumClass.GetValues()) };
			case MemberType::UINT16: return { .type = "uint16", .values = generate_enum_values(enumClass.GetValues()) };
			case MemberType::SINT32: return { .type = "int32", .values = generate_enum_values(enumClass.GetValues()) };
			case MemberType::UINT32: return { .type = "uint32", .values = generate_enum_values(enumClass.GetValues()) };
			case MemberType::SINT64: return { .type = "int64", .values = generate_enum_values(enumClass.GetValues()) };
			case MemberType::UINT64: return { .type = "uint64", .values = generate_enum_values(enumClass.GetValues()) };
			default: assert(false && "invalid member type"); return {};
			}
		}

		std::string get_enum(const GameInterface::RflSystem::RflClassEnum& enumClass, const std::string& cur_namespace, MemberType type) {
			std::string namespaced_name = cur_namespace + enumClass.GetName();

			if (templ.enums.contains(namespaced_name))
				return namespaced_name;

			templ.enums[namespaced_name] = generate_enum(enumClass, namespaced_name, type);

			return namespaced_name;
		}

		json_reflections::MemberDef generate_member(const GameInterface::RflSystem::RflClassMember& member, const std::string& cur_namespace) {
			auto type = get_type(member, cur_namespace);

			return {
				.name = member.GetName(),
				.type = type.type,
				.subtype = type.subtype,
				.flags = type.flag_values,
				.array_size = type.array_size,
				.alignment = type.alignment,
				.range = type.range,
				.descriptions = generate_descs(member.GetCaption()),
			};
		}

		bool has_member(const std::string& name, const GameInterface::RflSystem::RflClass& rflClass) {
			for (auto& member : rflClass.GetMembers())
				if (name == member.GetName())
					return true;

			auto* parent = rflClass.GetParent();
			if (parent)
				return has_member(name, *parent);

			return false;
		}

		std::string get_struct(const GameInterface::RflSystem::RflClass& rflClass) {
			std::string name = rflClass.GetName();

			if (templ.structs.contains(name))
				return name;

			auto [resIt, resSuccess] = templ.structs.emplace(name, json_reflections::StructDef{});
			auto& res = resIt->second;

			auto* parent = rflClass.GetParent();
			auto cur_namespace = name + std::string{ "::" };

			if (parent)
				res.parent = get_struct(*parent);

			std::vector<json_reflections::MemberDef> fields{};
			for (auto& member : rflClass.GetMembers()) {
				auto field = generate_member(member, cur_namespace);

				if (parent && has_member(field.name, *parent))
					field.name = cur_namespace + field.name;

				fields.push_back(std::move(field));
			}

			res.fields = std::move(fields);

			return name;
		}

	public:
		const Template& get_template() {
			return templ;
		}

		void add_class(const GameInterface::RflSystem::RflClass& rfl_class) {
			get_struct(rfl_class);
		}
	};

	template<typename GameInterface, HSONFormat format>
	class hson_template_builder : public template_builder<GameInterface> {
		static_assert(GameInterface::RflSystem::TypeSet::supports_old_array || format != HSONFormat::GEDIT_V2, "HSON format must not be GEDIT_V2 if type system does not support old array");
		static_assert(GameInterface::RflSystem::TypeSet::supports_old_array || format != HSONFormat::SOBJ_V1, "HSON format must not be SOBJ_V1 if type system does not support old array");
		static_assert(GameInterface::RflSystem::TypeSet::template supports_primitive<ucsl::objectids::ObjectIdV1> || format != HSONFormat::GEDIT_V2, "HSON format must not be GEDIT_V2 if type system does not support object ID v1");
		static_assert(GameInterface::RflSystem::TypeSet::template supports_primitive<ucsl::objectids::ObjectIdV1> || format != HSONFormat::SOBJ_V1, "HSON format must not be SOBJ_V1 if type system does not support object ID v1");
		static_assert(GameInterface::RflSystem::TypeSet::template supports_primitive<ucsl::objectids::ObjectIdV2> || format != HSONFormat::GEDIT_V3, "HSON format must not be V3 if type system does not support object ID v2");

		std::string get_object(const GameInterface::GameObjectClass& object) {
			std::string name = object.GetName();

			if (this->templ.objects.contains(name))
				return name;

			auto* cat = static_cast<const char*>(object.GetAttributeValue("category"));
			this->templ.objects.emplace(name, json_reflections::ObjectDef{ object.GetSpawnerDataClass() ? std::make_optional(this->get_struct(*object.GetSpawnerDataClass())) : std::nullopt, cat ? std::make_optional(cat) : std::nullopt});

			return name;
		}

		std::string get_tag(const GameInterface::GOComponentRegistry::GOComponentRegistryItem& component) {
			std::string name = component.GetName();

			if (this->templ.tags.contains(name))
				return name;

			this->templ.tags.emplace(name, json_reflections::TagDef{ component.GetSpawnerDataClass() ? std::make_optional(this->get_struct(*component.GetSpawnerDataClass())) : std::nullopt });

			return name;
		}

	public:
		hson_template_builder() {
			this->templ.version = 1;
			this->templ.format = format == HSONFormat::GEDIT_V3 ? "gedit_v3" : "gedit_v2";
		}

		void add_object(const GameInterface::GameObjectClass& object) {
			this->get_object(object);
		}

		void add_component(const GameInterface::GOComponentRegistry::GOComponentRegistryItem& component) {
			this->get_tag(component);
		}

		void add_all() {
			auto* gos = GameInterface::GameObjectSystem::GetInstance();

			for (auto* object : gos->gameObjectRegistry->GetGameObjectClasses())
				add_object(*object);

			for (auto* component : gos->goComponentRegistry->GetComponents())
				add_component(*component);
		}
	};
	
	template<typename GameInterface>
	class rfl_template_builder : public template_builder<GameInterface> {
	public:
		rfl_template_builder() {
			this->templ.version = 1;
			this->templ.format = "rfl";
		}

		void add_all() {
			for (auto* item : GameInterface::RflClassNameRegistry::GetInstance()->GetItems())
				this->add_class(*item);
		}
	};

	Template load(const std::string& filename) {
		auto json = rfl::json::load<Template>(filename);
		auto err = json.error();
		if (err.has_value())
			std::cout << err.value().what() << std::endl;
		return json.value();
	}

	void write(const std::string& filename, const Template& templ) {
		std::ofstream ofs{ filename, std::ios::trunc };
		rfl::json::write(templ, ofs, YYJSON_WRITE_PRETTY_TWO_SPACES);
	}
}
