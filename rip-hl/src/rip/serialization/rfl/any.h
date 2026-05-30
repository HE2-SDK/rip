#pragma once
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <rip/serialization/json.h>
#include "./forwards.h"

namespace rip::serialization {
	// Temporary lazy implementation by serializing to json then converting.
	template<typename T, bool arrayVectors = false>
	struct rfl {
		inline static ::rfl::Generic save(const T& model) {
			auto* doc = yyjson_mut_doc_new(nullptr);
			auto* val = json<T, arrayVectors>::save(doc, model);

			yyjson_mut_doc_set_root(doc, val);

			auto* idoc = yyjson_mut_doc_imut_copy(doc, nullptr);
			auto* iroot = yyjson_doc_get_root(idoc);

			auto result = ::rfl::json::read<::rfl::Generic>(iroot);

			yyjson_doc_free(idoc);
			yyjson_mut_doc_free(doc);

			return result.value();
		}

		inline static T load(const ::rfl::Generic& value) {
			auto val = ::rfl::json::write(value);

			yyjson_read_err err;
			auto* idoc = yyjson_read_opts(const_cast<char*>(val.c_str()), val.size(), 0, nullptr, &err);
			auto* iroot = yyjson_doc_get_root(idoc);

			auto result = json<T, arrayVectors>::load(idoc, iroot);

			yyjson_doc_free(idoc);

			return result;
		}
	};
}