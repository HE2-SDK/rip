#pragma once
#include <ucsl-reflection/game-interfaces/standalone/game-interface.h>
#include <filesystem>
#include <string>

using GI = ucsl::reflection::game_interfaces::standalone::StandaloneGameInterface;

enum class Format {
	BINARY,
	JSON,
	HSON,
};

enum class ResourceType {
	ASM,
	GEDIT,
	MAP,
	MATERIAL,
	RFL,
	VAT,
	FXCOL,
	SVCOL,
	SWIF,
	SOBJ,
	PCMODEL,
	NXS,
	PATH,
	MASTER_LEVEL,
	DENSITY_SETTING,
	AISM,
	CEMT,
};

enum class AddressingMode {
	_32,
	_64,
};

struct Config {
	std::filesystem::path inputFile{ "input.rfl" };
	std::filesystem::path outputFile{};
	std::optional<ResourceType> resourceType{};
	std::optional<std::string> version{};
	std::optional<Format> inputFormat{};
	std::optional<Format> outputFormat{};
	std::filesystem::path schema{};
	std::filesystem::path hedgesetTemplate{};
	AddressingMode addressingMode{ AddressingMode::_64 };
	static std::string rflClass;

	ResourceType getResourceType() const;
	Format getInputFormat() const;
	Format getOutputFormat() const;
	std::filesystem::path getOutputFile() const;
	void validate() const;
};
