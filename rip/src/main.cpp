#include <config.h>
#include <convert.h>
#include <util.h>
#include <CLI/CLI.hpp>
#include <iostream>
#include <map>
#undef SYNCHRONIZE
#undef VOID
#include <rip/schemas/hedgeset.h>

std::map<std::string, Format> formatMap{
	{ "binary", Format::BINARY },
	{ "json", Format::JSON },
	{ "hson", Format::HSON },
};

std::map<std::string, ResourceType> resourceTypeMap{
	{ "asm", ResourceType::ASM },
	{ "gedit", ResourceType::GEDIT },
	{ "map", ResourceType::MAP },
	{ "path", ResourceType::PATH },
	{ "material", ResourceType::MATERIAL },
	{ "rfl", ResourceType::RFL },
	{ "vat", ResourceType::VAT },
	{ "fxcol", ResourceType::FXCOL },
	{ "swif", ResourceType::SWIF },
	{ "sobj", ResourceType::SOBJ },
	{ "nxs", ResourceType::NXS },
	{ "pcmodel", ResourceType::PCMODEL },
	{ "masterlevel", ResourceType::MASTER_LEVEL },
	{ "densitysetting", ResourceType::DENSITY_SETTING },
	{ "aism", ResourceType::AISM },
};

std::map<std::string, AddressingMode> addressingModeMap{
	{ "32", AddressingMode::_32 },
	{ "64", AddressingMode::_64 },
};

auto formatMapReverse = reverse_map(formatMap);
auto resourceTypeMapReverse = reverse_map(resourceTypeMap);
auto addressingModeMapReverse = reverse_map(addressingModeMap);

int main(int argc, char** argv) {
	CLI::App app{ "Restoration Issue Pocketknife" };
	argv = app.ensure_utf8(argv);
	
	Config config{};

	app.add_option("input", config.inputFile, "The input file.")
		->required()
		->check(CLI::ExistingFile);
	app.add_option("output", config.outputFile, "The output file.");
	app.add_option("-r,--resource-type", config.resourceType, "The resource type.")
		->transform(CLI::CheckedTransformer(resourceTypeMap, CLI::ignore_case));
	auto* version = app.add_option("-v,--version", config.version, "The resource version. Available options are: asm -> 103; gedit -> 2, 3; vat -> 1-rangers, 1-miller; fxcol -> 1");
	app.add_option("-i,--input-format", config.inputFormat, "The input format.")
		->transform(CLI::CheckedTransformer(formatMap, CLI::ignore_case));
	app.add_option("-o,--output-format", config.outputFormat, "The output format.")
		->transform(CLI::CheckedTransformer(formatMap, CLI::ignore_case));
	app.add_option("-t,--hedgeset-template", config.hedgesetTemplate, "The HedgeSet template file to use.");
	app.add_option("-c,--rfl-class", Config::rflClass, "When converting RFL files: the name of the RflClass to use.");
	app.validate_positionals();

	CLI11_PARSE(app, argc, argv);

	try {
		config.validate();

		std::cerr << "Converting " << resourceTypeMapReverse[config.getResourceType()] << " from " << formatMapReverse[config.getInputFormat()] << " to " << formatMapReverse[config.getOutputFormat()] << std::endl;
		std::cerr << "Input file: " << config.inputFile.generic_string() << std::endl;
		std::cerr << "Output file: " << config.getOutputFile().generic_string() << std::endl;

		ucsl::reflection::game_interfaces::standalone::StandaloneGameInterface::boot();

		if (!config.hedgesetTemplate.empty()) {
			auto templ = rip::schemas::hedgeset::load(config.hedgesetTemplate.generic_string());
			rip::schemas::hedgeset::schema_builder s{ templ };
			GI::reflectionDB->load_schema(s.get_schema());
		}

		rip::cli::convert::convert(config);

		//std::ifstream ifs{ config.inputFile, std::ios::binary | std::ios::ate };
		//size_t fileSize = ifs.tellg();

		//std::unique_ptr<uint8_t[]> fileData = std::make_unique<uint8_t[]>(fileSize);

		//ifs.seekg(std::ios::beg);
		//ifs.read((char*)fileData.get(), fileSize);

		//auto* doc = yyjson_mut_doc_new(nullptr);

		//rip::binary::mem_istream mis{ fileData.get() };
		//rip::binary::binary_istream<rip::binary::mem_istream, uint64_t> bis{ mis };

		//using Refl = ucsl::reflection::providers::simplerfl<GI>::RootType<ucsl::resources::animation_state_machine::v103_rangers::reflections::AsmData>;
		//using Model = rip::models::BinaryFileV2<Refl, GI::AllocatorSystem>;

		//auto model = rip::serialization::binary<Model>::load(bis);
		//auto* result = rip::serialization::json<Model>::save(doc, model);

		//yyjson_mut_doc_set_root(doc, result);

		//yyjson_write_err err;
		//std::string filename = config.getOutputFile().generic_string();
		//yyjson_mut_write_file(filename.c_str(), doc, YYJSON_WRITE_PRETTY_TWO_SPACES | YYJSON_WRITE_ALLOW_INF_AND_NAN | YYJSON_WRITE_ALLOW_INVALID_UNICODE, nullptr, &err);

		//if (err.code != YYJSON_WRITE_SUCCESS) {
		//	std::cerr << "Error writing json: " << err.msg << std::endl;
		//}

		//yyjson_mut_doc_free(doc);





		////std::ifstream ifs{ config.inputFile, std::ios::binary | std::ios::ate };
		////size_t fileSize = ifs.tellg();

		////std::unique_ptr<uint8_t[]> fileData = std::make_unique<uint8_t[]>(fileSize);

		////ifs.seekg(std::ios::beg);
		////ifs.read((char*)&fileData[0], fileSize);

		////imemstream ims{ (char*)&fileData[0], fileSize };

		//yyjson_doc* doc = yyjson_read_file(config.inputFile.generic_string().c_str(), 0, nullptr, nullptr);

		//ucsl::reflection::providers::simplerfl<GI>::RootType<ucsl::resources::cemt::v100000::reflections::EffectParam> refl{};

		////rip::binary::fast_istream fis{ ims };
		////rip::binary::binary_istream<size_t> bis{ fis };
		////rip::accessors::binary_istream<decltype(bis)>::ValueAccessor<decltype(refl)> acc{ bis, refl };

		//rip::binary::accessors::json<false>::ValueAccessor<decltype(refl)> acc{ doc, refl };

		//std::ofstream ofs{ config.getOutputFile().generic_string(), std::ios::binary };
		//rip::binary::fast_ostream fos{ ofs };
		//rip::binary::binary_ostream<size_t> bos{ fos, 0 };
		//rip::binary::BinarySerializer serialize{ bos };
		//serialize.process_root(acc);

		////yyjson_mut_doc* doc = yyjson_mut_doc_new(nullptr);
		////rip::binary::SerializeJson<false> serialize{ doc };
		////yyjson_mut_val* result = serialize.process(acc);

		////yyjson_mut_doc_set_root(doc, result);

		////yyjson_write_err err;
		////std::string filename = config.getOutputFile().generic_string();
		////yyjson_mut_write_file(filename.c_str(), doc, YYJSON_WRITE_PRETTY_TWO_SPACES | YYJSON_WRITE_ALLOW_INF_AND_NAN | YYJSON_WRITE_ALLOW_INVALID_UNICODE, nullptr, &err);

		////if (err.code != YYJSON_WRITE_SUCCESS) {
		////	std::cerr << "Error writing json: " << err.msg << std::endl;
		////}

		////yyjson_mut_doc_free(doc);



		std::cerr << "Conversion successful." << std::endl;
	}
	catch (std::runtime_error& e) {
		std::cerr << e.what();
		return 1;
	}

	return 0;
}
