#pragma once
#include <ucsl/rfl/rflclass.h>
#include <ucsl/resources/material/v3.h>
#include <ucsl/resources/map/v1.h>
#include <ucsl/resources/path/v1.h>
#include <ucsl/resources/sobj/v1.h>
#include <ucsl/resources/nxs/v1.h>
#include <ucsl/resources/swif/v5.h>
#include <ucsl/resources/swif/v6.h>
#include <config.h>
#include "BinaryInputFile.h"
#include "MirageInputFile.h"
#include "SWIFInputFile.h"
#include "JsonInputFile.h"

template<typename T>
InputFile<T>* loadInputFile(const Config& config) {
	switch (config.getInputFormat()) {
	case Format::BINARY:
		if constexpr (std::is_same_v<T, ucsl::resources::swif::v5::SRS_PROJECT> || std::is_same_v<T, ucsl::resources::swif::v6::SRS_PROJECT>)
			return new SWIFInputFile<T>{ config };
		else if constexpr (std::is_same_v<T, ucsl::resources::map::v1::MapData<GI::AllocatorSystem>>)
			return new BinaryInputFileV1<T, size_t>{ config };
		else if constexpr (std::is_same_v<T, ucsl::resources::sobj::v1::SetObjectData<GI::AllocatorSystem>>)
			return new BinaryInputFileV1<T, size_t>{ config };
		else if constexpr (std::is_same_v<T, ucsl::resources::nxs::v1::NXSData>)
			return new BinaryInputFileV1<T, size_t>{ config };
		else if constexpr (std::is_same_v<T, ucsl::resources::path::v1::PathsData>)
			return new BinaryInputFileV1<T, size_t>{ config };
		else if constexpr (std::is_same_v<T, ucsl::resources::material::contexts::ContextsData>) {
			if (config.version == "1")
				return new MirageInputFileV1<T, size_t>{ config };
			else
				return new MirageInputFileV2<T, size_t>{ config };
		}
		else
			return new BinaryInputFileV2<T, size_t>{ config };
	case Format::JSON: return new JsonInputFile<T>{ config };
	default: assert("unknown input format"); return nullptr;
	}
}
