#pragma once
#include <rip/serialization/hson/common.h>
#include <ranges>
#include <optional>

namespace rip::serialization::hson_internal {
	inline reflections::hson::File saveGedit(const reflections::gedit::ObjectWorldData& data) {
		auto transformedObjects = std::views::all(data.objects) | std::views::transform([](const auto& obj) {
			auto hasParent = obj.parentID != "{00000000-0000-0000-0000-000000000000}";

			::rfl::Object<::rfl::Generic> tags{};
			for (const auto& componentData : obj.componentData) {
				tags[componentData.type] = componentData.data;
			}

			auto localPosition = hasParent ? obj.localTransform.position : obj.transform.position;
			auto localRotation = hasParent ? obj.localTransform.rotation : obj.transform.rotation;
			auto localQuatRotation = util::eulerToQuat(Eigen::Vector3f{ localRotation.data() });

			return reflections::hson::Object{
				.id = obj.id,
				.name = obj.name,
				.parentId = hasParent ? std::make_optional(obj.parentID) : std::nullopt,
				.type = obj.gameObjectClass,
				.position = localPosition,
				.rotation = std::array<float, 4>{ localQuatRotation.x(), localQuatRotation.y(), localQuatRotation.z(), localQuatRotation.w() },
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

		for (auto& obj : file.objects) {
			auto pos = obj.position.value();
			auto rot = obj.rotation.value();
			auto rotEuler = util::quatToEuler(Eigen::Quaternionf{ rot[3], rot[0], rot[1], rot[2] });

			reflections::gedit::ObjectTransformData localTransform{ std::array<float, 3>{ pos[0], pos[1], pos[2] }, std::array<float, 3>{ rotEuler[0], rotEuler[1], rotEuler[2] } };
			reflections::gedit::ObjectTransformData transform{ localTransform };

			if (hasParent(obj)) {
				auto tf = getAbsoluteTransform(obj, file.objects);
				auto absPos = tf.translation();
				auto absRot = util::matrixToEuler(tf.rotation());

				transform.position = { absPos.x(), absPos.y(), absPos.z() };
				transform.rotation = { absRot.x(), absRot.y(), absRot.z() };
			}

			if (!obj.id.has_value())
				throw std::runtime_error{ "Object without an ID was found. While this is allowed by the HSON spec, RIP currently does not support it." };

			const auto objectId = obj.id.value();

			std::vector<reflections::gedit::ComponentData> resultComponents{};

			const auto* params = getObjectProperty<reflections::hson::Parameters>(obj, file.objects, [](const reflections::hson::Object& o) -> const std::optional<reflections::hson::Parameters>&{ return o.parameters; });

			if (params == nullptr)
				throw std::runtime_error{ std::format("Could not find parameters for object {}", obj.id.value()) };

			if (auto& components = params->tags) {
				for (auto& [type, component] : components.value()) {
					auto* componentRflClass = GameInterface::GameObjectSystem::GetInstance()->goComponentRegistry->GetComponentInformationByName(type.c_str())->GetSpawnerDataClass();

					if (componentRflClass == nullptr)
						throw std::runtime_error{ std::format("Object {} refers to a component type `{}`, which is unknown.", obj.id.value(), type) };

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
				throw std::runtime_error{ std::format("Object {} does not have a type.", obj.id.value()) };

			resultObjs.emplace_back(reflections::gedit::ObjectData{
				.gameObjectClass = *gameObjectClass,
				.name = obj.name.value(),
				.id = obj.id.value(), // TODO: id is not required to exist. generate one if it doesn't
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
