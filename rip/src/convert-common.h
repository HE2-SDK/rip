#pragma once
#include <fstream>
#include <config.h>
#include <rip/serialization/binary.h>
#include <rip/serialization/json.h>
#include "resource-table.h"

namespace rip::cli::convert {
	template<typename T, typename AddrType, std::endian endianness>
	T loadVersion(const Config& config) {
		switch (config.getInputFormat()) {
		case Format::BINARY: {
			std::ifstream ifs{ config.inputFile, std::ios::binary | std::ios::ate };

			size_t size = ifs.tellg();
			std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(size);

			ifs.seekg(std::ios::beg);
			ifs.read((char*)data.get(), size);

			rip::binary::mem_istream mis{ data.get() };
			rip::binary::binary_istream<rip::binary::mem_istream, AddrType> bis{ mis, endianness };

			return rip::serialization::binary<T>::load(bis);
		}
		case Format::JSON: {
			yyjson_read_err err;
			yyjson_doc* doc = yyjson_read_file(config.inputFile.generic_string().c_str(), 0, nullptr, &err);
			if (err.code != YYJSON_READ_SUCCESS)
				throw std::runtime_error{ std::string{ "Error reading json: " } + err.msg };

			auto result = rip::serialization::json<T>::load(doc, yyjson_doc_get_root(doc));

			yyjson_doc_free(doc);

			return result;
		}
		default:
			return {};
		}
	}

	template<typename T, typename AddrType, std::endian endianness>
	void saveVersion(const Config& config, const T& model) {
		switch (config.getOutputFormat()) {
		case Format::BINARY: {
			std::ofstream ofs{ config.getOutputFile(), std::ios::binary | std::ios::trunc};

			rip::binary::fast_ostream fos{ ofs };
			rip::binary::binary_ostream<rip::binary::fast_ostream, AddrType, endianness> bos{ fos };

			rip::serialization::binary<T>::save(bos, model);

			break;
		}
		case Format::JSON: {
			auto* doc = yyjson_mut_doc_new(nullptr);

			auto* result = rip::serialization::json<T>::save(doc, model);

			yyjson_mut_doc_set_root(doc, result);

			yyjson_write_err err;
			yyjson_mut_write_file(config.getOutputFile().generic_string().c_str(), doc, YYJSON_WRITE_PRETTY_TWO_SPACES | YYJSON_WRITE_ALLOW_INF_AND_NAN | YYJSON_WRITE_ALLOW_INVALID_UNICODE, nullptr, &err);

			if (err.code != YYJSON_WRITE_SUCCESS) {
				std::runtime_error stdErr{ std::string{ "Error writing json: " } + err.msg };
				
				yyjson_mut_doc_free(doc);

				throw stdErr;
			}

			yyjson_mut_doc_free(doc);

			break;
		}
		default:
			break;
		}
	}

	template<typename T, typename AddrType, std::endian endianness>
	void convertVersion(const Config& config) {
		T model = loadVersion<T, AddrType, endianness>(config);
		saveVersion<T, AddrType, endianness>(config, model);
	}

	template<ResourceType type, strlit defaultVersion, typename... Versions>
	void convertVersions(const Config& config, resources::resource<type, defaultVersion, Versions...>) {
		std::string defVer = defaultVersion;
		std::string version = config.version.value_or(defVer);

		if (!((version == Versions::name.operator std::string() && (convertVersion<typename Versions::resourceDef, typename Versions::addrType, Versions::endianness>(config), true)) || ...))
			throw std::runtime_error{ std::string{ "Version " } + version + " is invalid for selected resource type." };
	}
}
