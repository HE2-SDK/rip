#include "convert.h"
#include "convert-impl.h"
#include <tuple>

namespace rip::cli::convert {
	template<ResourceType type, strlit defaultVersion, typename... Versions>
	void convertVersions(const Config& config, resources::resource<type, defaultVersion, Versions...>) {
		std::string defVer = defaultVersion;
		std::string version = config.version.value_or(defVer);

		if (!((version == Versions::name.operator std::string() && (convertNamedVersion<resources::resource<type, defaultVersion, Versions...>, Versions::name>(config), true)) || ...))
			throw std::runtime_error{ std::string{ "Version " } + version + " is invalid for selected resource type." };
	}

	template<typename Resource>
	void convertResource(const Config& config, Resource resource) {
		convertVersions(config, resource);
	}

	template<typename... Resources>
	void convertResources(const Config& config, std::tuple<Resources...>) {
		auto resourceType = config.getResourceType();

		if (!((resourceType == Resources::type && (convertResource(config, Resources{}), true)) || ...))
			throw std::runtime_error{ "Resource type does not exist." };
	}

	void convert(const Config& config) {
		convertResources(config, resources::all{});
	}
}
