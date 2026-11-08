#pragma once
#include <ucsl-reflection/providers/simplerfl.h>
#include <ucsl-reflection/reflections/resources/sobj/v1.h>
#include <rip/models/binary-file/v2.h>
#include <rip/serialization/hson/common.h>
#include <rip/serialization/rfl/any.h>

namespace rip::serialization {
	namespace hson_internal {
		inline bool isSobjInstanceObject(const reflections::hson::Object& object) {
			return object.instanceOf.has_value()
				&& !object.name.has_value()
				&& !object.parameters.has_value()
				&& !object.parentId.has_value()
				&& !object.type.has_value();
		}

		struct SOBJObjectContext {
			reflections::sobj::ObjectData object{};
			std::string objType{};
		};

		inline reflections::sobj::ObjectData convertHsonToSobjObject(const reflections::hson::Object& object, const std::vector<reflections::hson::Object>& objects) {
			const auto& params = *getObjectProperty<reflections::hson::Parameters>(object, objects, [](const reflections::hson::Object& o) -> const std::optional<reflections::hson::Parameters>&{ return o.parameters; });
			const auto rangeSpawning = params.tags.value().get("RangeSpawning").value().to_object().value();

			return {
				.id = object.id.value(),
				.objectClassId = 0,
				.bvhNode = 0,
				.replicationInterval = 0.0f,
				.m_distance = static_cast<float>(rangeSpawning.get("rangeIn").value().to_double().value()),
				.m_range = static_cast<float>(rangeSpawning.get("rangeOut").value().to_double().value()),
				.instances = {},
				.spawnerData = params.parameters,
			};
		}
	}

	template<typename GameInterface>
	struct hson<GameInterface, models::BinaryFileV1<typename ucsl::reflection::providers::simplerfl<GameInterface>::template RootType<typename ucsl::resources::sobj::v1::reflections::SetObjectData<typename GameInterface::AllocatorSystem>>, typename GameInterface::AllocatorSystem, true>> {
		typedef models::BinaryFileV1<typename ucsl::reflection::providers::simplerfl<GameInterface>::template RootType<ucsl::resources::sobj::v1::reflections::SetObjectData<typename GameInterface::AllocatorSystem>>, typename GameInterface::AllocatorSystem, true> Model;

		inline static void save(std::ostream& stream, const Model& model) {
			auto rflData = rfl<Model, true>::save(model);
			hson_internal::reflections::sobj::SetObjectData data = ::rfl::from_generic<hson_internal::reflections::sobj::SetObjectData>(rflData).value();

			std::vector<hson_internal::reflections::hson::Object> result{};
			std::mt19937 mt{ std::random_device{}() };

			for (auto& type : data.objectTypes) {
				for (auto idx : type.objectIndices) {
					auto& obj = data.objects[idx];
					//auto id = util::toGUID(ucsl::objectids::ObjectIdV1{ obj.id.id & 0x0000FFFF });

					::rfl::Object<::rfl::Generic> rangeSpawning{};
					rangeSpawning["rangeIn"] = obj.m_distance;
					rangeSpawning["rangeOut"] = obj.m_range;

					::rfl::Object<::rfl::Generic> tags{};
					tags["RangeSpawning"] = rangeSpawning;

					if (obj.instances.size() > 1) {
						result.push_back({
							.id = obj.id,
							.type = type.name,
							.isExcluded = true,
							.parameters = hson_internal::reflections::hson::Parameters{
								.tags = tags,
								.parameters = { obj.spawnerData },
							},
						});

						for (auto& instance : obj.instances) {
							result.push_back({
								.id = util::toGUID(ucsl::objectids::ObjectIdV1{ mt() }),
								.instanceOf = obj.id,
								.position = instance.position,
								.rotation = hson_internal::eulerToQuat(instance.rotation),
							});
						}
					}
					else {
						auto& instance = obj.instances[0];

						result.push_back({
							.id = obj.id,
							.type = type.name,
							.position = instance.position,
							.rotation = hson_internal::eulerToQuat(instance.rotation),
							.parameters = hson_internal::reflections::hson::Parameters{
								.tags = tags,
								.parameters = { obj.spawnerData },
							},
						});
					}
				}
			}

			return hson_internal::writeHSON(stream, hson_internal::generateHSONFile(std::move(result)));
		}

		inline static Model load(std::istream& stream) {
			auto file = hson_internal::readHSON(stream);

			std::map<std::string, hson_internal::SOBJObjectContext> resultObjectsById{};
			std::map<std::string, hson_internal::reflections::sobj::ObjectTypeData> resultObjectTypesByName{};
			std::mt19937 mt{ std::random_device{}() };
			unsigned int objInstanceCount{};

			for (auto& obj : file.objects) {
				if (obj.isExcluded.value_or(false))
					continue;

				const auto objectId = obj.id.has_value() ? obj.id.value() : util::toGUID(ucsl::objectids::ObjectIdV2{ mt(), mt() });

				hson_internal::reflections::sobj::ObjectTransformData transform{ obj.position.value(), hson_internal::quatToEuler(obj.rotation.value()) };

				const auto* objType = hson_internal::getObjectProperty<std::string>(obj, file.objects, [](const hson_internal::reflections::hson::Object& o) -> const std::optional<std::string>&{ return o.type; });

				if (objType == nullptr)
					throw std::runtime_error{ std::format("Object {} does not have a type.", objectId) };

				if (hson_internal::isSobjInstanceObject(obj)) {
					auto classId = obj.instanceOf.value();
					auto it = resultObjectsById.find(classId);

					if (it != resultObjectsById.end()) {
						auto& objCtx = it->second;

						objCtx.object.instances.emplace_back(transform);
					}
					else {
						auto* classObj = hson_internal::findObject(classId, file.objects);

						if (classObj == nullptr)
							throw std::runtime_error{ std::format("Object {} refers to unknown class object {}.", objectId, classId) };

						auto& objCtx = resultObjectsById[objectId] = { .object = hson_internal::convertHsonToSobjObject(*classObj, file.objects), .objType = *objType };

						objCtx.object.instances.emplace_back(transform);
					}
				}
				else {
					auto& objCtx = resultObjectsById[objectId] = { .object = hson_internal::convertHsonToSobjObject(obj, file.objects), .objType = *objType };

					objCtx.object.instances.emplace_back(transform);
				}

				objInstanceCount++;
			}

			size_t idx{};
			for (const auto& obj : resultObjectsById) {
				auto& objType = resultObjectTypesByName[obj.second.objType];

				objType.objectIndices.emplace_back(idx++);
			}

			for (auto& objType : resultObjectTypesByName) {
				objType.second.name = objType.first;
				objType.second.objectIndexCount = objType.second.objectIndices.size();
			}

			std::vector<hson_internal::reflections::sobj::ObjectData> resultObjects{};
			std::ranges::copy(std::views::values(resultObjectsById) | std::views::transform([](hson_internal::SOBJObjectContext& ctx) { return ctx.object; }), std::back_inserter(resultObjects));

			std::vector<hson_internal::reflections::sobj::ObjectTypeData> resultObjectTypes{};
			std::ranges::copy(std::views::values(resultObjectTypesByName), std::back_inserter(resultObjectTypes));

			hson_internal::reflections::sobj::SetObjectData data{
				.magic = 0x534F424A,
				.version = 1,
				.objectTypeCount = static_cast<unsigned int>(resultObjectTypes.size()),
				.objectTypes = std::move(resultObjectTypes),
				.bvh = -1,
				.objects = std::move(resultObjects),
				.objectCount = static_cast<unsigned int>(resultObjects.size()),
				.bvhNodeCount = 0,
				.objectInstanceCount = objInstanceCount,
			};

			auto rflData = ::rfl::to_generic(data);

			return rfl<Model, true>::load(rflData);
		}
	};
}
