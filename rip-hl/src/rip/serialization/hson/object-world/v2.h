#pragma once
#include <ucsl-reflection/providers/simplerfl.h>
#include <ucsl-reflection/reflections/resources/object-world/v2.h>
#include <rip/models/binary-file/v2.h>
#include <rip/serialization/hson/common.h>
#include <rip/serialization/rfl/any.h>
#include "./common.h"

namespace rip::serialization {
	template<typename GameInterface>
	struct hson<GameInterface, models::BinaryFileV2<typename ucsl::reflection::providers::simplerfl<GameInterface>::template RootType<typename ucsl::resources::object_world::v2::reflections::ObjectWorldData<typename GameInterface::AllocatorSystem>>, typename GameInterface::AllocatorSystem>> {
		typedef models::BinaryFileV2<typename ucsl::reflection::providers::simplerfl<GameInterface>::template RootType<ucsl::resources::object_world::v2::reflections::ObjectWorldData<typename GameInterface::AllocatorSystem>>, typename GameInterface::AllocatorSystem> Model;

		inline static void save(std::ostream& stream, const Model& model) {
			auto rflData = rfl<Model, true>::save(model);
			auto data = ::rfl::from_generic<hson_internal::reflections::gedit::ObjectWorldData>(rflData).value();

			return hson_internal::writeHSON(stream, hson_internal::saveGedit(data));
		}

		inline static Model load(std::istream& stream) {
			auto file = hson_internal::readHSON(stream);
			auto data = hson_internal::loadGedit<GameInterface>(file);
			auto rflData = ::rfl::to_generic(data);

			return rfl<Model, true>::load(rflData);
		}
	};
}
