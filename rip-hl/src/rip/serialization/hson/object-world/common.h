#pragma once
#include <rip/serialization/hson/common.h>
#include <ranges>
#include <optional>

namespace rip::serialization::hson_internal {
	inline reflections::hson::File saveGedit(const reflections::gedit::ObjectWorldData& data) {
		auto transformedObjects = std::views::all(data.objects) | std::views::transform([](const auto& obj) {
			const auto hasParent = obj.parentID != "{00000000-0000-0000-0000-000000000000}";

			::rfl::Object<::rfl::Generic> tags{};
			for (const auto& componentData : obj.componentData) {
				tags[componentData.type] = componentData.data;
			}

			const auto localPosition = hasParent ? obj.localTransform.position : obj.transform.position;
			const auto localRotation = hasParent ? obj.localTransform.rotation : obj.transform.rotation;

			return reflections::hson::Object{
				.id = obj.id,
				.name = obj.name,
				.parentId = hasParent ? std::make_optional(obj.parentID) : std::nullopt,
				.type = obj.gameObjectClass,
				.position = localPosition,
				.rotation = hson_internal::eulerToQuat(localRotation),
				.parameters = reflections::hson::Parameters{
					.tags = tags,
					.parameters = { obj.spawnerData },
				},
			};
		});

		return generateHSONFile(transformedObjects);
	}

	template<typename GameInterface>
	inline reflections::gedit::ObjectWorldData loadGedit(const reflections::hson::File& file) {
		std::vector<reflections::gedit::ObjectData> resultObjs{};
		std::mt19937_64 mt{ std::random_device{}() };

		for (const auto& obj : file.objects) {
			reflections::gedit::ObjectTransformData localTransform{ obj.position.value(), hson_internal::quatToEuler(obj.rotation.value()) };
			reflections::gedit::ObjectTransformData transform{ localTransform };

			if (hasParent(obj)) {
				auto tf = getAbsoluteTransform(obj, file.objects);
				auto absPos = tf.translation();
				auto absRot = util::matrixToEuler(tf.rotation());

				transform.position = { absPos.x(), absPos.y(), absPos.z() };
				transform.rotation = { absRot.x(), absRot.y(), absRot.z() };
			}

			const auto objectId = obj.id.has_value() ? obj.id.value() : util::toGUID(ucsl::objectids::ObjectIdV2{ mt(), mt() });

			std::vector<reflections::gedit::ComponentData> resultComponents{};

			const auto* params = getObjectProperty<reflections::hson::Parameters>(obj, file.objects, [](const reflections::hson::Object& o) -> const std::optional<reflections::hson::Parameters>&{ return o.parameters; });

			if (params == nullptr)
				throw std::runtime_error{ std::format("Could not find parameters for object {}", objectId) };

			if (auto& components = params->tags) {
				for (auto& [type, component] : components.value()) {
					auto* componentRflClass = GameInterface::GameObjectSystem::GetInstance()->goComponentRegistry->GetComponentInformationByName(type.c_str())->GetSpawnerDataClass();

					if (componentRflClass == nullptr)
						throw std::runtime_error{ std::format("Object {} refers to a component type `{}`, which is unknown.", objectId, type) };

					resultComponents.emplace_back(reflections::gedit::ComponentData{
						.type = type,
						.size = componentRflClass->GetSize(),
						.data = component,
					});
				}
			}

			const auto* parentId = getObjectProperty<std::string>(obj, file.objects, [](const reflections::hson::Object& o) { return o.parentId; });
			const auto* gameObjectClass = getObjectProperty<std::string>(obj, file.objects, [](const reflections::hson::Object& o) { return o.type; });

			if (gameObjectClass == nullptr)
				throw std::runtime_error{ std::format("Object {} does not have a type.", objectId) };

			resultObjs.emplace_back(reflections::gedit::ObjectData{
				.gameObjectClass = *gameObjectClass,
				.name = obj.name.value(),
				.id = objectId,
				.parentID = parentId == nullptr ? "{00000000-0000-0000-0000-000000000000}" : *parentId,
				.transform = transform,
				.localTransform = localTransform,
				.componentData = std::move(resultComponents),
				.spawnerData = params->parameters,
			});
		}

		return { resultObjs };
	}
}
