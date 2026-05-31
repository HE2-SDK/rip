#pragma once
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <optional>
#include <ctime>
#include <rip/util/math.h>
#include <rip/util/object-id-guids.h>
#include <random>

namespace rip::serialization::hson_internal {
	namespace reflections {
		namespace hson {
			struct Parameters {
				std::optional<::rfl::Object<::rfl::Generic>> tags{};
				::rfl::ExtraFields<::rfl::Generic> parameters{};
			};

			struct Object {
				std::optional<std::string> id{};
				std::optional<std::string> name{};
				std::optional<std::string> parentId{};
				std::optional<std::string> instanceOf{};
				std::optional<std::string> type{};
				std::optional<std::array<float, 3>> position{};
				std::optional<std::array<float, 4>> rotation{};
				std::optional<std::array<float, 3>> scale{};
				std::optional<bool> isEditorVisible{};
				std::optional<bool> isExcluded{};
				std::optional<Parameters> parameters{};
			};

			struct Metadata {
				std::optional<std::string> name{};
				std::optional<std::string> author{};
				std::optional<std::string> date{};
				std::optional<std::string> version{};
				std::optional<std::string> description{};
			};

			struct File {
				::rfl::Rename<"$schema", std::string> schema{};
				unsigned int version{};
				std::optional<Metadata> metadata{};
				std::vector<Object> objects{};
			};
		}

		namespace gedit {
			struct ObjectTransformData {
				std::array<float, 3> position;
				std::array<float, 3> rotation;
			};

			struct ComponentData {
				std::string type;
				unsigned long long size;
				::rfl::Generic data;
			};

			struct ObjectData {
				std::string gameObjectClass;
				std::string name;
				std::string id;
				std::string parentID;
				ObjectTransformData transform;
				ObjectTransformData localTransform;
				std::vector<ComponentData> componentData;
				::rfl::Object<::rfl::Generic> spawnerData;
			};

			struct ObjectWorldData {
				std::vector<ObjectData> objects;
			};
		}

		namespace sobj {
			struct ObjectTransformData {
				std::array<float, 3> position;
				std::array<float, 3> rotation;
			};

			struct ObjectData {
				std::string id;
				unsigned int objectClassId;
				unsigned int bvhNode;
				float replicationInterval;
				float m_distance;
				float m_range;
				std::vector<ObjectTransformData> instances;
				::rfl::Object<::rfl::Generic> spawnerData;
			};

			struct ObjectTypeData {
				std::string name;
				unsigned int objectIndexCount;
				std::vector<unsigned short> objectIndices;
			};

			struct SetObjectData {
				unsigned int magic;
				unsigned int version;
				unsigned int objectTypeCount;
				std::vector<ObjectTypeData> objectTypes;
				int bvh;
				std::vector<ObjectData> objects;
				unsigned int objectCount;
				unsigned int bvhNodeCount;
				unsigned int objectInstanceCount;
			};
		}
	}

	inline std::array<float, 4> eulerToQuat(const std::array<float, 3>& vec) {
		auto quat = util::eulerToQuat(Eigen::Vector3f{ vec.data() });

		return std::array<float, 4>{ quat.x(), quat.y(), quat.z(), quat.w() };
	}

	inline std::array<float, 3> quatToEuler(const std::array<float, 4>& quat) {
		auto vec = util::quatToEuler(Eigen::Quaternionf{ quat.data() });

		return std::array<float, 3>{ vec.x(), vec.y(), vec.z() };
	}

	inline bool hasParent(const reflections::hson::Object& object) {
		return object.parentId.has_value() && object.parentId.value() != "{00000000-0000-0000-0000-000000000000}";
	}

	inline const reflections::hson::Object* findObject(const std::string& id, const std::vector<reflections::hson::Object>& objects) {
		auto res = std::find_if(objects.begin(), objects.end(), [&id](const auto& o) { return o.id == id; });

		return res == objects.end() ? nullptr : &*res;
	}

	inline Eigen::Affine3f getAbsoluteTransform(const reflections::hson::Object& object, const std::vector<reflections::hson::Object>& objects) {
		auto pos = object.position.value();
		auto rot = object.rotation.value();

		Eigen::Affine3f localTransform = Eigen::Translation3f{ pos[0], pos[1], pos[2] } * Eigen::Quaternionf{ rot[3], rot[0], rot[1], rot[2] };

		if (!hasParent(object))
			return localTransform;

		auto* parent = findObject(object.parentId.value(), objects);

		if (parent == nullptr)
			throw std::runtime_error{ std::format("unable to calculate transform for object {}: can't find parent object {}", object.id.value(), object.parentId.value())};

		return getAbsoluteTransform(*parent, objects) * localTransform;
	}

	template<typename T, typename F>
	inline const T* getObjectProperty(const reflections::hson::Object& object, const std::vector<reflections::hson::Object>& objects, F f) {
		const std::optional<T>& res{ f(object) };

		if (res.has_value())
			return &res.value();

		if (!object.instanceOf.has_value())
			return nullptr;

		auto* parent = findObject(object.parentId.value(), objects);

		if (parent == nullptr)
			throw std::runtime_error{ std::format("unable to get properties for object {}: can't find parent object {}", object.id.value(), object.parentId.value()) };

		return getObjectProperty<T>(*parent, objects, std::forward<decltype(f)>(f));
	}

	inline reflections::hson::File generateHSONFile(std::ranges::input_range auto&& objects) {
		std::time_t now = std::time(nullptr);
		std::string date = std::asctime(std::localtime(&now));

		return {
			.schema = "https://raw.githubusercontent.com/hedge-dev/hson-schema/main/hson.schema.json",
			.version = 1,
			.metadata = reflections::hson::Metadata{
				.author = "Restoration Issue Pocketknife",
				.date = date.substr(0, date.size() - 1),
				.version = "1.0.0",
				.description = "Converted by Restoration Issue Pocketknife.",
			},
			.objects = { objects.begin(), objects.end() },
		};
	}

	inline void writeHSON(std::ostream& stream, const reflections::hson::File& file) {
		::rfl::json::write(file, stream, YYJSON_WRITE_PRETTY_TWO_SPACES | YYJSON_WRITE_ALLOW_INF_AND_NAN | YYJSON_WRITE_ALLOW_INVALID_UNICODE);
	}

	inline reflections::hson::File readHSON(std::istream& stream) {
		auto res = ::rfl::json::read<reflections::hson::File>(stream);

		if (res.error().has_value())
			throw std::runtime_error{ std::format("Could not parse HSON. Error: {}", res.error().value().what()) };

		return res.value();
	}
}
