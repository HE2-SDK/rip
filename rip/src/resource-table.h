#pragma once
#include <ucsl-reflection/reflections/resources/asm/v103-rangers.h>
#include <ucsl-reflection/reflections/resources/asm/v103-miller.h>
#include <ucsl-reflection/providers/simplerfl.h>
//#include <ucsl-reflection/reflections/resources/fxcol/v1.h>
#include <ucsl-reflection/reflections/resources/svcol/v1.h>
//#include <ucsl-reflection/reflections/resources/map/v1.h>
#include <ucsl-reflection/reflections/resources/material/v3.h>
//#include <ucsl-reflection/reflections/resources/object-world/v2.h>
//#include <ucsl-reflection/reflections/resources/object-world/v3.h>
//#include <ucsl-reflection/reflections/resources/rfl/v1.h>
//#include <ucsl-reflection/reflections/resources/rfl/v2.h>
#include <ucsl-reflection/reflections/resources/vertex-animation-texture/v1-rangers.h>
#include <ucsl-reflection/reflections/resources/vertex-animation-texture/v1-miller.h>
//#include <ucsl-reflection/reflections/resources/swif/v5.h>
//#include <ucsl-reflection/reflections/resources/swif/v6.h>
//#include <ucsl-reflection/reflections/resources/sobj/v1.h>
//#include <ucsl-reflection/reflections/resources/nxs/v1.h>
//#include <ucsl-reflection/reflections/resources/path/v1.h>
//#include <ucsl-reflection/reflections/resources/path/v200.h>
//#include <ucsl-reflection/reflections/resources/pointcloud/v2.h>
//#include <ucsl-reflection/reflections/resources/master-level/v0.h>
//#include <ucsl-reflection/reflections/resources/density-setting/v11.h>
//#include <ucsl-reflection/reflections/resources/aism/v0.h>
#include <rip/models/binary-file/v2.h>
#include <rip/models/mirage/v1.h>
#include <rip/models/mirage/v2.h>
#include <tuple>
#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include "config.h"

//inline const char* get_rfl1_class(const ucsl::resources::rfl::v1::Ref1Data<>& parent) { return Config::rflClass.c_str(); }
//inline const char* get_rfl2_class(const ucsl::resources::rfl::v2::Ref2Data<>& parent) { return Config::rflClass.c_str(); }
//
//namespace simplerfl {
//	template<> struct canonical<ucsl::resources::rfl::v1::Ref1Data<>> { using type = ucsl::resources::rfl::v1::reflections::Ref1Data<ucsl::resources::rfl::v1::Ref1RflData, get_rfl1_class>; };
//	template<> struct canonical<ucsl::resources::rfl::v2::Ref2Data<>> { using type = ucsl::resources::rfl::v2::reflections::Ref2Data<ucsl::resources::rfl::v2::Ref2RflData, get_rfl2_class>; };
//}

namespace rip::cli::convert {
	template <size_t N>
	struct strlit {
		std::array<char, N> buffer{};

		constexpr strlit(const auto... characters) : buffer{ characters..., '\0' } {}
		constexpr strlit(const std::array<char, N> buffer) : buffer{ buffer } {}
		constexpr strlit(const char(&str)[N]) { std::copy_n(str, N, std::data(buffer)); }

		operator const char* () const { return std::data(buffer); }
		operator std::string() const { return std::string(std::data(buffer), N - 1); }
		constexpr operator std::string_view() const { return std::string_view(std::data(buffer), N - 1); }
	};

	namespace resources {
		template<ResourceType type_, strlit defaultVersion_, typename... Versions> struct resource {
			static constexpr ResourceType type = type_;
			static constexpr strlit defaultVersion = defaultVersion_;
			using versions = std::tuple<Versions...>;
		};
		template<strlit name_, typename ResourceDef, typename AddrType, std::endian endianness_> struct version {
			static constexpr strlit name = name_;
			using resourceDef = ResourceDef;
			using addrType = AddrType;
			static constexpr std::endian endianness = endianness_;
		};

		using animation_state_machine = resource<ResourceType::ASM, "1.03-miller",
			version<"1.03-rangers", rip::models::BinaryFileV2<ucsl::reflection::providers::simplerfl<GI>::RootType<ucsl::resources::animation_state_machine::v103_rangers::reflections::AsmData>, GI::AllocatorSystem>, uint64_t, std::endian::little>,
			version<"1.03-miller", rip::models::BinaryFileV2<ucsl::reflection::providers::simplerfl<GI>::RootType<ucsl::resources::animation_state_machine::v103_miller::reflections::AsmData>, GI::AllocatorSystem>, uint64_t, std::endian::little>
		>;

		//using gedit = resource<ResourceType::GEDIT, "3",
		//	version<"2", ucsl::resources::object_world::v2::ObjectWorldData<GI::AllocatorSystem>>,
		//	version<"3", ucsl::resources::object_world::v3::ObjectWorldData<GI::AllocatorSystem>>
		//>;

		//using map = resource<ResourceType::MAP, "1",
		//	version<"1", ucsl::resources::map::v1::MapData<GI::AllocatorSystem>>
		//>;

		using material = resource<ResourceType::MATERIAL, "3",
			version<"3-container-v1", rip::models::MirageContainerV1<ucsl::reflection::providers::simplerfl<GI>::RootType<ucsl::resources::material::v3::reflections::ContextsData>, GI::AllocatorSystem>, uint32_t, std::endian::big>,
			version<"3", rip::models::MirageContainerV2<ucsl::reflection::providers::simplerfl<GI>::RootType<ucsl::resources::material::v3::reflections::ContextsData>, GI::AllocatorSystem>, uint32_t, std::endian::big>
		>;

		//using rfl = resource<ResourceType::RFL, "2-1.00",
		//	version<"1", ucsl::resources::rfl::v1::Ref1Data<>>,
		//	version<"2-1.00", ucsl::resources::rfl::v2::Ref2Data<>>
		//>;

		using vat = resource<ResourceType::VAT, "1-miller",
			version<"1-rangers", rip::models::BinaryFileV2<ucsl::reflection::providers::simplerfl<GI>::RootType<ucsl::resources::vertex_animation_texture::v1_rangers::reflections::VertexAnimationTextureData>, GI::AllocatorSystem>, uint64_t, std::endian::little>,
			version<"1-miller", rip::models::BinaryFileV2<ucsl::reflection::providers::simplerfl<GI>::RootType<ucsl::resources::vertex_animation_texture::v1_miller::reflections::VertexAnimationTextureData>, GI::AllocatorSystem>, uint64_t, std::endian::little>
		>;

		//using fxcol = resource<ResourceType::FXCOL, "1",
		//	version<"1", rip::models::BinaryFileV2<ucsl::reflection::providers::simplerfl<GI>::RootType<ucsl::resources::fxcol::v1::reflections::FxColData>, GI::AllocatorSystem>, uint64_t, std::endian::little>
		//>;

		using svcol = resource < ResourceType::SVCOL, "1",
			version<"1", rip::models::BinaryFileV2<ucsl::reflection::providers::simplerfl<GI>::RootType<ucsl::resources::svcol::v1::reflections::SvColData>, GI::AllocatorSystem>, uint64_t, std::endian::little>
		>;

		//using swif = resource<ResourceType::SWIF, "6",
		//	version<"5", ucsl::resources::swif::v5::SRS_PROJECT>,
		//	version<"6", ucsl::resources::swif::v6::SRS_PROJECT>
		//>;
		//using sobj = resource<ResourceType::SOBJ, "1",
		//	version<"1", ucsl::resources::sobj::v1::SetObjectData<GI::AllocatorSystem>>
		//>;
		//using nxs = resource<ResourceType::NXS, "1",
		//	version<"1", ucsl::resources::nxs::v1::NXSData>
		//>;
		//using path = resource<ResourceType::PATH, "2.00",
		//	version<"1", ucsl::resources::path::v1::PathsData>,
		//	version<"2.00", ucsl::resources::path::v200::PathsData>
		//>;
		//using pcmodel = resource<ResourceType::PCMODEL, "2",
		//	version<"2", ucsl::resources::pointcloud::v2::PointcloudData>
		//>;
		//using mlevel = resource<ResourceType::MASTER_LEVEL, "0",
		//	version<"0", ucsl::resources::master_level::v0::MasterLevelData>
		//>;
		//using density_setting = resource<ResourceType::DENSITY_SETTING, "11",
		//	version<"11", ucsl::resources::density_setting::v11::DensitySettingData>
		//>;
		//using aism = resource<ResourceType::AISM, "0",
		//	version<"0", ucsl::resources::aism::v0::AIStateMachineData>
		//>;

		using all = std::tuple<
			animation_state_machine,
			//gedit,
			//map,
			material,
			//rfl,
			vat,
			//fxcol,
			svcol
			//swif,
			//sobj,
			//nxs,
			//path,
			//pcmodel,
			//mlevel,
			//density_setting,
			//aism
		>;
	}
}
