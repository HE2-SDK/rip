#pragma once
#include <type_traits>
#include <ucsl-reflection/util/memory.h>
#include <ucsl-reflection/providers/types.h>
#include <ucsl-reflection/accessors/types.h>
#include <rip/util/object-id-guids.h>
#include <yyjson.h>

namespace rip::binary::accessors {
	template<bool arrayVectors = false>
	struct json_mut {
		template<typename Refl> class ValueAccessor;

		template<typename Refl>
		class AccessorBase {
		public:
			Refl refl;

			inline AccessorBase(const Refl& refl) : refl{ refl } {}
		};

		struct Reference {
			enum class Kind {
				CUSTOM,
				ROOT,
				PROPERTY,
				INDEX,
			};

			struct RootRef {};
			struct PropertyRef {
				yyjson_mut_val* obj{};
				const char* key{};
			};
			struct IndexRef {
				yyjson_mut_val* arr{};
				size_t index{};
			};

			union Ref {
				RootRef root;
				PropertyRef property;
				IndexRef index;
			};

			yyjson_mut_doc* doc{};
			yyjson_mut_val* val{};
			Kind kind{};
			Ref ref{};

			Reference(yyjson_mut_doc* doc, yyjson_mut_val* val) : doc{ doc }, val{ val }, kind{ Kind::CUSTOM } {}
			Reference(yyjson_mut_doc* doc) : doc{ doc }, val{ yyjson_mut_doc_get_root(doc) }, kind{ Kind::ROOT } {}
			Reference(const Reference& parent, const char* key) : doc{ parent.doc }, val{ yyjson_mut_obj_get(parent, key) }, kind{ Kind::PROPERTY }, ref{ .property = { .obj = parent, .key = key } } {}
			Reference(const Reference& parent, size_t index, yyjson_mut_val* val = nullptr) : doc{ parent.doc }, val{ val ? val : yyjson_mut_arr_get(parent, index) }, kind{ Kind::INDEX }, ref{ .index = { .arr = parent, .index = index } } {}


			Reference& operator=(yyjson_mut_val* v) {
				switch (kind) {
				case Kind::CUSTOM: val = v; break;
				case Kind::ROOT: yyjson_mut_doc_set_root(doc, v); val = v; break;
				case Kind::PROPERTY: yyjson_mut_obj_replace(ref.property.obj, yyjson_mut_str(doc, ref.property.key), v); val = v; break;
				case Kind::INDEX: yyjson_mut_arr_replace(ref.index.arr, ref.index.index, v); val = v; break;
				}
				return *this;
			}

			operator yyjson_mut_val*() const {
				if (val != nullptr)
					return val;

				switch (kind) {
				case Kind::CUSTOM: assert("reading from custom json value that was not provided");
				case Kind::ROOT: return yyjson_mut_doc_get_root(doc);
				case Kind::PROPERTY: return yyjson_mut_obj_get(ref.property.obj, ref.property.key);
				case Kind::INDEX: return yyjson_mut_arr_get(ref.index.arr, ref.index.index);
				}

				return nullptr;
			}
		};

		template<typename Refl>
		class Accessor : public AccessorBase<Refl> {
		public:
			Reference reference{};

			inline Accessor(Reference reference, const Refl& refl = Refl{}) : AccessorBase<Refl>{ refl }, reference{ reference } {}
		};

		template<typename Refl, typename Repr = typename Refl::repr>
		class PrimitiveDataAccessor;
		template<typename Refl, std::signed_integral T>
		class PrimitiveDataAccessor<Refl, T> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				this->reference = yyjson_mut_sint(this->reference.doc, this->refl.is_erased ? typename Refl::repr{} : this->refl.constant_value.has_value() ? this->refl.constant_value.value() : v);
				return *this;
			}

			operator typename Refl::repr () const {
				return this->refl.is_erased ? typename Refl::repr{} : this->refl.constant_value.has_value() ? this->refl.constant_value.value() : yyjson_mut_get_sint(this->reference);
			}
		};
		template<typename Refl, std::unsigned_integral T>
		class PrimitiveDataAccessor<Refl, T> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				this->reference = yyjson_mut_uint(this->reference.doc, this->refl.is_erased ? typename Refl::repr{} : this->refl.constant_value.has_value() ? this->refl.constant_value.value() : v);
				return *this;
			}

			operator typename Refl::repr () const {
				return this->refl.is_erased ? typename Refl::repr{} : this->refl.constant_value.has_value() ? this->refl.constant_value.value() : yyjson_mut_get_uint(this->reference);
			}
		};
		template<typename Refl, std::floating_point T>
		class PrimitiveDataAccessor<Refl, T> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				this->reference = yyjson_mut_num(this->reference.doc, this->refl.is_erased ? typename Refl::repr{} : this->refl.constant_value.has_value() ? this->refl.constant_value.value() : v);
				return *this;
			}

			operator typename Refl::repr () const {
				return this->refl.is_erased ? typename Refl::repr{} : this->refl.constant_value.has_value() ? this->refl.constant_value.value() : static_cast<typename Refl::repr>(yyjson_mut_get_num(this->reference));
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, bool> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				this->reference = yyjson_mut_bool(this->reference.doc, this->refl.is_erased ? typename Refl::repr{} : this->refl.constant_value.has_value() ? this->refl.constant_value.value() : v);
				return *this;
			}

			operator typename Refl::repr () const {
				return this->refl.is_erased ? typename Refl::repr{} : this->refl.constant_value.has_value() ? this->refl.constant_value.value() : static_cast<typename Refl::repr>(yyjson_mut_get_bool(this->reference));
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Vector2> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				if constexpr (arrayVectors) {
					yyjson_mut_val* res = yyjson_mut_arr(this->reference.doc);
					for (size_t i = 0; i < v.rows(); i++)
						for (size_t j = 0; j < v.cols(); j++)
							yyjson_mut_arr_add_float(this->reference.doc, res, v(i, j));
					this->reference = res;
				}
				else {
					yyjson_mut_val* res = yyjson_mut_obj(this->reference.doc);
					yyjson_mut_obj_add_float(this->reference.doc, res, "x", v.x());
					yyjson_mut_obj_add_float(this->reference.doc, res, "y", v.y());
					this->reference = res;
				}
				return *this;
			}

			operator typename Refl::repr () const {
				if constexpr (arrayVectors) {
					typename Refl::repr res{};

					assert(yyjson_mut_arr_size(this->reference) == 2);
					size_t i, max;
					yyjson_mut_val* item;
					yyjson_mut_arr_foreach(this->reference, i, max, item) {
						res(i, 0) = yyjson_mut_get_num(item);
					}

					return res;
				}
				else {
					return {
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "x"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "y"))),
					};
				}
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Vector3> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				if constexpr (arrayVectors) {
					yyjson_mut_val* res = yyjson_mut_arr(this->reference.doc);
					for (size_t i = 0; i < v.rows(); i++)
						for (size_t j = 0; j < v.cols(); j++)
							yyjson_mut_arr_add_float(this->reference.doc, res, v(i, j));
					this->reference = res;
				}
				else {
					yyjson_mut_val* res = yyjson_mut_obj(this->reference.doc);
					yyjson_mut_obj_add_float(this->reference.doc, res, "x", v.x());
					yyjson_mut_obj_add_float(this->reference.doc, res, "y", v.y());
					yyjson_mut_obj_add_float(this->reference.doc, res, "z", v.z());
					this->reference = res;
				}
			}

			operator typename Refl::repr () const {
				if constexpr (arrayVectors) {
					typename Refl::repr res{};

					assert(yyjson_mut_arr_size(this->reference) == 3);
					size_t i, max;
					yyjson_mut_val* item;
					yyjson_mut_arr_foreach(this->reference, i, max, item) {
						res(i, 0) = yyjson_mut_get_num(item);
					}

					return res;
				}
				else {
					return {
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "x"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "y"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "z"))),
					};
				}
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Vector4> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				if constexpr (arrayVectors) {
					yyjson_mut_val* res = yyjson_mut_arr(this->reference.doc);
					for (size_t i = 0; i < v.rows(); i++)
						for (size_t j = 0; j < v.cols(); j++)
							yyjson_mut_arr_add_float(this->reference.doc, res, v(i, j));
					this->reference = res;
				}
				else {
					yyjson_mut_val* res = yyjson_mut_obj(this->reference.doc);
					yyjson_mut_obj_add_float(this->reference.doc, res, "x", v.x());
					yyjson_mut_obj_add_float(this->reference.doc, res, "y", v.y());
					yyjson_mut_obj_add_float(this->reference.doc, res, "z", v.z());
					yyjson_mut_obj_add_float(this->reference.doc, res, "w", v.w());
					this->reference = res;
				}
			}

			operator typename Refl::repr () const {
				if constexpr (arrayVectors) {
					typename Refl::repr res{};

					assert(yyjson_mut_arr_size(this->reference) == 4);
					size_t i, max;
					yyjson_mut_val* item;
					yyjson_mut_arr_foreach(this->reference, i, max, item) {
						res(i, 0) = yyjson_mut_get_num(item);
					}

					return res;
				}
				else {
					return {
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "x"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "y"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "z"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "w"))),
					};
				}
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Quaternion> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				if constexpr (arrayVectors) {
					yyjson_mut_val* res = yyjson_mut_arr(this->reference.doc);
					for (size_t i = 0; i < v.rows(); i++)
						for (size_t j = 0; j < v.cols(); j++)
							yyjson_mut_arr_add_float(this->reference.doc, res, v(i, j));
					this->reference = res;
				}
				else {
					yyjson_mut_val* res = yyjson_mut_obj(this->reference.doc);
					yyjson_mut_obj_add_float(this->reference.doc, res, "x", v.x());
					yyjson_mut_obj_add_float(this->reference.doc, res, "y", v.y());
					yyjson_mut_obj_add_float(this->reference.doc, res, "z", v.z());
					yyjson_mut_obj_add_float(this->reference.doc, res, "w", v.w());
					this->reference = res;
				}
			}

			operator typename Refl::repr () const {
				if constexpr (arrayVectors) {
					typename Refl::repr res{};

					assert(yyjson_mut_arr_size(this->reference) == 4);
					size_t i, max;
					yyjson_mut_val* item;
					yyjson_mut_arr_foreach(this->reference, i, max, item) {
						res.coeffs()(i, 0) = yyjson_mut_get_num(item);
					}

					return res;
				}
				else {
					return {
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "w"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "x"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "y"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "z"))),
					};
				}
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Position> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				if constexpr (arrayVectors) {
					yyjson_mut_val* res = yyjson_mut_arr(this->reference.doc);
					for (size_t i = 0; i < v.rows(); i++)
						for (size_t j = 0; j < v.cols(); j++)
							yyjson_mut_arr_add_float(this->reference.doc, res, v(i, j));
					this->reference = res;
				}
				else {
					yyjson_mut_val* res = yyjson_mut_obj(this->reference.doc);
					yyjson_mut_obj_add_float(this->reference.doc, res, "x", v.x());
					yyjson_mut_obj_add_float(this->reference.doc, res, "y", v.y());
					yyjson_mut_obj_add_float(this->reference.doc, res, "z", v.z());
					this->reference = res;
				}
			}

			operator typename Refl::repr () const {
				if constexpr (arrayVectors) {
					typename Refl::repr res{};

					assert(yyjson_mut_arr_size(this->reference) == 3);
					size_t i, max;
					yyjson_mut_val* item;
					yyjson_mut_arr_foreach(this->reference, i, max, item) {
						res(i, 0) = yyjson_mut_get_num(item);
					}

					return res;
				}
				else {
					return {
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "x"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "y"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "z"))),
					};
				}
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Rotation> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				if constexpr (arrayVectors) {
					yyjson_mut_val* res = yyjson_mut_arr(this->reference.doc);
					for (size_t i = 0; i < v.rows(); i++)
						for (size_t j = 0; j < v.cols(); j++)
							yyjson_mut_arr_add_float(this->reference.doc, res, v(i, j));
					this->reference = res;
				}
				else {
					yyjson_mut_val* res = yyjson_mut_obj(this->reference.doc);
					yyjson_mut_obj_add_float(this->reference.doc, res, "x", v.x());
					yyjson_mut_obj_add_float(this->reference.doc, res, "y", v.y());
					yyjson_mut_obj_add_float(this->reference.doc, res, "z", v.z());
					yyjson_mut_obj_add_float(this->reference.doc, res, "w", v.w());
					this->reference = res;
				}
			}

			operator typename Refl::repr () const {
				if constexpr (arrayVectors) {
					typename Refl::repr res{};

					assert(yyjson_mut_arr_size(this->reference) == 4);
					size_t i, max;
					yyjson_mut_val* item;
					yyjson_mut_arr_foreach(this->reference, i, max, item) {
						res.coeffs()(i, 0) = yyjson_mut_get_num(item);
					}

					return res;
				}
				else {
					return {
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "w"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "x"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "y"))),
						static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "z"))),
					};
				}
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Matrix34> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				yyjson_mut_val* res = yyjson_mut_arr(this->reference.doc);
				for (size_t i = 0; i < v.rows(); i++)
					for (size_t j = 0; j < v.cols(); j++)
						yyjson_mut_arr_add_float(this->reference.doc, res, v(i, j));
				this->reference = res;
			}

			operator typename Refl::repr () const {
				typename Refl::repr res{};

				assert(yyjson_mut_arr_size(this->reference) == 12);
				size_t i, max;
				yyjson_mut_val* item;
				yyjson_mut_arr_foreach(this->reference, i, max, item) {
					res(i / 4, i % 4) = yyjson_mut_get_num(item);
				}

				return res;
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Matrix44> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				yyjson_mut_val* res = yyjson_mut_arr(this->reference.doc);
				for (size_t i = 0; i < v.rows(); i++)
					for (size_t j = 0; j < v.cols(); j++)
						yyjson_mut_arr_add_float(this->reference.doc, res, v(i, j));
				this->reference = res;
			}

			operator typename Refl::repr () const {
				typename Refl::repr res{};

				assert(yyjson_mut_arr_size(this->reference) == 16);
				size_t i, max;
				yyjson_mut_val* item;
				yyjson_mut_arr_foreach(this->reference, i, max, item) {
					res(i / 4, i % 4) = yyjson_mut_get_num(item);
				}

				return res;
			}
		};
		template<typename Refl, ucsl::colors::ChannelOrder order>
		class PrimitiveDataAccessor<Refl, ucsl::colors::Color8<order>> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				yyjson_mut_val* res = yyjson_mut_obj(this->reference.doc);
				yyjson_mut_obj_add_uint(this->reference.doc, res, "r", v.r);
				yyjson_mut_obj_add_uint(this->reference.doc, res, "g", v.g);
				yyjson_mut_obj_add_uint(this->reference.doc, res, "b", v.b);
				yyjson_mut_obj_add_uint(this->reference.doc, res, "a", v.a);
				this->reference = res;
			}

			operator typename Refl::repr () const {
				typename Refl::repr res{};
				
				res.r = static_cast<uint8_t>(yyjson_mut_get_uint(yyjson_mut_obj_get(this->reference, "r")));
				res.g = static_cast<uint8_t>(yyjson_mut_get_uint(yyjson_mut_obj_get(this->reference, "g")));
				res.b = static_cast<uint8_t>(yyjson_mut_get_uint(yyjson_mut_obj_get(this->reference, "b")));
				res.a = static_cast<uint8_t>(yyjson_mut_get_uint(yyjson_mut_obj_get(this->reference, "a")));

				return res;
			}
		};
		template<typename Refl, ucsl::colors::ChannelOrder order>
		class PrimitiveDataAccessor<Refl, ucsl::colors::Colorf<order>> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				yyjson_mut_val* res = yyjson_mut_obj(this->reference.doc);
				yyjson_mut_obj_add_float(this->reference.doc, res, "r", v.r);
				yyjson_mut_obj_add_float(this->reference.doc, res, "g", v.g);
				yyjson_mut_obj_add_float(this->reference.doc, res, "b", v.b);
				yyjson_mut_obj_add_float(this->reference.doc, res, "a", v.a);
				this->reference = res;
			}

			operator typename Refl::repr () const {
				typename Refl::repr res{};
				
				res.r = static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "r")));
				res.g = static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "g")));
				res.b = static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "b")));
				res.a = static_cast<float>(yyjson_mut_get_num(yyjson_mut_obj_get(this->reference, "a")));

				return res;
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::objectids::ObjectIdV1> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				char guid[39];
				util::toGUID(v, guid);
				this->reference = yyjson_mut_strcpy(this->reference.doc, guid);
			}

			operator typename Refl::repr () const {
				return util::fromGUID<typename Refl::repr>(yyjson_mut_get_str(this->reference));
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::objectids::ObjectIdV2> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				char guid[39];
				util::toGUID(v, guid);
				this->reference = yyjson_mut_strcpy(this->reference.doc, guid);
			}

			operator typename Refl::repr () const {
				return util::fromGUID<typename Refl::repr>(yyjson_mut_get_str(this->reference));
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::strings::VariableString> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				this->reference = yyjson_mut_str(this->reference.doc, v.c_str());
			}

			operator typename Refl::repr () const {
				return yyjson_mut_get_str(this->reference);
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, const char*> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			PrimitiveDataAccessor<Refl>& operator=(typename Refl::repr v) {
				this->reference = yyjson_mut_str(this->reference.doc, v);
			}

			operator typename Refl::repr () const {
				return yyjson_mut_get_str(this->reference);
			}
		};

		template<typename Refl>
		class PrimitiveAccessor : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			template<typename F>
			inline auto visit(F f) {
				return this->refl.visit([&](auto r){ return f(PrimitiveDataAccessor<decltype(r)>{ this->reference, r }); });
			}

			template<typename F>
			inline const auto visit(F f) const {
				return this->refl.visit([&](auto r){ return f(PrimitiveDataAccessor<decltype(r)>{ this->reference, r }); });
			}
		};

		template<typename Refl>
		class EnumAccessor : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;
			
			EnumAccessor<Refl>& operator=(long long v) {
				this->reference = yyjson_mut_sint(this->reference.doc, v);
				return *this;
			}

			operator long long () const {
				if (yyjson_mut_is_int(this->reference)) {
					return yyjson_mut_get_sint(this->reference);
				}
				if (yyjson_mut_is_str(this->reference)) {
					const char* str = yyjson_mut_get_str(this->reference);

					for (auto& option : this->refl.get_options())
						if (!strcmp(option.GetEnglishName(), str))
							return option.GetIndex();
				}
				assert("unhandled enum");
				return -1;
			}
		};

		template<typename Refl>
		class StructureAccessor : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			template<typename FieldRefl>
			inline auto operator[](const FieldRefl& field_refl) {
				auto type = field_refl.get_type(*this);

				return ValueAccessor<decltype(type)>{ Reference{ this->reference, field_refl.get_name() }, type };
			}

			template<typename FieldRefl>
			inline const auto operator[](const FieldRefl& field_refl) const {
				auto type = field_refl.get_type(*this);

				return ValueAccessor<decltype(type)>{ Reference{ this->reference, field_refl.get_name() }, type };
			}

			inline auto get_base() const {
				auto base = this->refl.get_base();

				return base.has_value() ? std::make_optional(StructureAccessor<std::remove_reference_t<decltype(base.value())>>{ this->reference, base.value() }) : std::nullopt;
			}
		};

		template<typename Refl>
		class CArrayAccessor : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			inline size_t get_length() const {
				return this->refl.get_length();
			}

			inline auto operator[](size_t idx) {
				auto item_refl = this->refl.get_item_type();

				assert(idx < this->refl.get_length());

				return ValueAccessor<decltype(item_refl)>{ { this->reference, idx }, item_refl };
			}

			inline const auto operator[](size_t idx) const {
				auto item_refl = this->refl.get_item_type();

				assert(idx < this->refl.get_length());

				return ValueAccessor<decltype(item_refl)>{ { this->reference, idx }, item_refl };
			}
		};

		template<typename Refl>
		class PointerAccessor : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			inline void clear() {
				this->reference = yyjson_mut_null(this->reference.doc);
			}

			inline void set(const Accessor<Refl>& other) {
				this->reference = other.obj;
			}

			inline auto get() {
				auto target_type = this->refl.get_target_type();

				return yyjson_mut_is_null(this->reference) ? std::nullopt : std::make_optional<ValueAccessor<decltype(target_type)>>({ this->reference, target_type });
			}

			inline auto get() const {
				auto target_type = this->refl.get_target_type();

				return yyjson_mut_is_null(this->reference) ? std::nullopt : std::make_optional<const ValueAccessor<decltype(target_type)>>({ this->reference, target_type });
			}
		};

		template<typename Refl>
		class ArrayAccessor : public Accessor<Refl> {
		public:
			class iterator {
				ArrayAccessor& accessor;
				yyjson_mut_val* cur;
				size_t idx{};

			public:
				iterator(ArrayAccessor& accessor, yyjson_mut_val* cur, size_t idx) : accessor{ accessor }, cur{ cur }, idx{ idx } {}
				iterator(const iterator& other) : accessor{ other.accessor }, cur{ other.cur }, idx{ other.idx } {}

				iterator& operator++() {
					cur = cur->next;
					idx++;
					return *this;
				}

				iterator operator++(int) {
					iterator result{ *this };
					cur = cur->next;
					idx++;
					return result;
				}

				bool operator==(const iterator& other) const { return idx == other.idx; }
				bool operator!=(const iterator& other) const { return idx != other.idx; }
				bool operator<(const iterator& other) const { return idx < other.idx; }
				bool operator>(const iterator& other) const { return idx > other.idx; }
				bool operator<=(const iterator& other) const { return idx <= other.idx; }
				bool operator>=(const iterator& other) const { return idx >= other.idx; }
				auto operator*() {
					auto item_type = accessor.refl.get_item_type();

					return ValueAccessor<decltype(item_type)>{ { accessor->reference, idx, cur }, item_type };
				}
			};

			class const_iterator {
				ArrayAccessor& accessor;
				yyjson_mut_val* cur;
				size_t idx{};

			public:
				const_iterator(ArrayAccessor& accessor, yyjson_mut_val* cur, size_t idx) : accessor{ accessor }, cur{ cur }, idx{ idx } {}
				const_iterator(const const_iterator& other) : accessor{ other.accessor }, cur{ other.cur }, idx{ other.idx } {}

				const_iterator& operator++() {
					cur = cur->next;
					idx++;
					return *this;
				}

				const_iterator operator++(int) {
					const_iterator result{ *this };
					cur = cur->next;
					idx++;
					return result;
				}

				bool operator==(const const_iterator& other) const { return idx == other.idx; }
				bool operator!=(const const_iterator& other) const { return idx != other.idx; }
				bool operator<(const const_iterator& other) const { return idx < other.idx; }
				bool operator>(const const_iterator& other) const { return idx > other.idx; }
				bool operator<=(const const_iterator& other) const { return idx <= other.idx; }
				bool operator>=(const const_iterator& other) const { return idx >= other.idx; }
				const auto operator*() const {
					auto item_type = accessor.refl.get_item_type();

					return ValueAccessor<decltype(item_type)>{ { accessor->reference, idx, cur }, item_type };
				}
			};

			using Accessor<Refl>::Accessor;

			//GameInterface::AllocatorSystem::allocator_type* get_allocator() const { return this->reference.get_allocator(); }
			//void change_allocator(GameInterface::AllocatorSystem::allocator_type* new_allocator) { this->reference.change_allocator(this->refl, new_allocator); }
			void reserve(size_t len) {}

			auto emplace_back() {
				// construct

				return (*this)[size() - 1];
			}
			void remove(size_t i) { yyjson_mut_arr_remove(this->reference, i); }
			void clear() { yyjson_mut_arr_clear(this->reference); }

			iterator begin() { return { *this, yyjson_mut_arr_get_first(this->reference), 0 }; }
			const_iterator begin() const { return { *this, yyjson_mut_arr_get_first(this->reference), 0 }; }
			const_iterator cbegin() const { return { *this, yyjson_mut_arr_get_first(this->reference), 0 }; }
			iterator end() { return { *this, nullptr, size() }; }
			const_iterator end() const { return { *this, nullptr, size() }; }
			const_iterator cend() const { return { *this, nullptr, size() }; }

			auto operator[](size_t i) {
				auto item_type = this->refl.get_item_type();

				return ValueAccessor<decltype(item_type)>{ { this->reference, i }, item_type };
			}
			const auto operator[](size_t i) const {
				auto item_type = this->refl.get_item_type();

				return ValueAccessor<decltype(item_type)>{ { this->reference, i }, item_type };
			}

			size_t size() const { return yyjson_mut_arr_size(this->reference); }
			size_t capacity() const { return yyjson_mut_arr_size(this->reference); }
		};

		template<typename Refl>
		class ValueAccessor : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			const auto visit(auto f) const {
				return this->refl.visit([&](auto r) {
					if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::PRIMITIVE) return f(PrimitiveAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ENUM) return f(EnumAccessor<decltype(r)>{ this->reference, r });
					//else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::FLAGS) return f(flags(this->reference, refl.is_erased(), r));
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ARRAY) return f(ArrayAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::TARRAY) return f(ArrayAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::POINTER) return f(PointerAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::CARRAY) return f(CArrayAccessor<decltype(r)>{ this->reference, r });
					//else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::UNION) return f(union(this->reference, parent, r));
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::STRUCTURE) return f(StructureAccessor<decltype(r)>{ this->reference, r });
					else static_assert(false, "invalid type kind");
				});
			}

			auto visit(auto f) {
				return this->refl.visit([&](auto r) {
					if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::PRIMITIVE) return f(PrimitiveAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ENUM) return f(EnumAccessor<decltype(r)>{ this->reference, r });
					//else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::FLAGS) return f(flags(this->reference, refl.is_erased(), r));
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ARRAY) return f(ArrayAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::TARRAY) return f(ArrayAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::POINTER) return f(PointerAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::CARRAY) return f(CArrayAccessor<decltype(r)>{ this->reference, r });
					//else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::UNION) return f(union(this->reference, parent, r));
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::STRUCTURE) return f(StructureAccessor<decltype(r)>{ this->reference, r });
					else static_assert(false, "invalid type kind");
				});
			}

			auto as_primitive() {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::PRIMITIVE) return PrimitiveAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a primitive"); });
			}

			auto as_enum() {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ENUM) return EnumAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not an enum"); });
			}

			auto as_array() {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ARRAY) return ArrayAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a array"); });
			}

			auto as_tarray() {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::TARRAY) return ArrayAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a tarray"); });
			}

			auto as_carray() {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::CARRAY) return CArrayAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a carray"); });
			}

			auto as_pointer() {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::POINTER) return PointerAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a pointer"); });
			}

			auto as_structure() {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::STRUCTURE) return StructureAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a structure"); });
			}
		};
	};


	template<bool arrayVectors = false>
	struct json {
		template<typename Refl> class ValueAccessor;

		template<typename Refl>
		class AccessorBase {
		public:
			Refl refl;

			inline AccessorBase(const Refl& refl) : refl{ refl } {}
		};

		struct Reference {
			enum class Kind {
				CUSTOM,
				ROOT,
				PROPERTY,
				INDEX,
			};

			struct RootRef {};
			struct PropertyRef {
				yyjson_val* obj{};
				const char* key{};
			};
			struct IndexRef {
				yyjson_val* arr{};
				size_t index{};
			};

			union Ref {
				RootRef root;
				PropertyRef property;
				IndexRef index;
			};

			yyjson_doc* doc{};
			yyjson_val* val{};
			Kind kind{};
			Ref ref{};
			
			Reference(yyjson_doc* doc, yyjson_val* val) : doc{ doc }, val{ val }, kind{ Kind::CUSTOM } {}
			Reference(yyjson_doc* doc) : doc{ doc }, val{ yyjson_doc_get_root(doc) }, kind{ Kind::ROOT } {}
			Reference(const Reference& parent, const char* key) : doc{ parent.doc }, val{ yyjson_obj_get(parent, key) }, kind{ Kind::PROPERTY }, ref{ .property = { .obj = parent, .key = key } } {}
			Reference(const Reference& parent, size_t index, yyjson_val* val = nullptr) : doc{ parent.doc }, val{ val ? val : yyjson_arr_get(parent, index) }, kind{ Kind::INDEX }, ref{ .index = { .arr = parent, .index = index } } {}

			operator yyjson_val*() const {
				if (val != nullptr)
					return val;

				switch (kind) {
				case Kind::CUSTOM: assert("reading from custom json value that was not provided");
				case Kind::ROOT: return yyjson_doc_get_root(doc);
				case Kind::PROPERTY: return yyjson_obj_get(ref.property.obj, ref.property.key);
				case Kind::INDEX: return yyjson_arr_get(ref.index.arr, ref.index.index);
				}

				return nullptr;
			}
		};

		template<typename Refl>
		class Accessor : public AccessorBase<Refl> {
		protected:
			Reference reference{};
		
		public:
			inline Accessor(Reference reference, const Refl& refl = Refl{}) : AccessorBase<Refl>{ refl }, reference{ reference } {}
		};

		template<typename Refl, typename Repr = typename Refl::repr>
		class PrimitiveDataAccessor;
		template<typename Refl, std::signed_integral T>
		class PrimitiveDataAccessor<Refl, T> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				return this->refl.is_erased ? typename Refl::repr{} : this->refl.constant_value.has_value() ? this->refl.constant_value.value() : yyjson_get_sint(this->reference);
			}
		};
		template<typename Refl, std::unsigned_integral T>
		class PrimitiveDataAccessor<Refl, T> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				return this->refl.is_erased ? typename Refl::repr{} : this->refl.constant_value.has_value() ? this->refl.constant_value.value() : yyjson_get_uint(this->reference);
			}
		};
		template<typename Refl, std::floating_point T>
		class PrimitiveDataAccessor<Refl, T> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				return this->refl.is_erased ? typename Refl::repr{} : this->refl.constant_value.has_value() ? this->refl.constant_value.value() : static_cast<typename Refl::repr>(yyjson_get_num(this->reference));
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, bool> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				return this->refl.is_erased ? typename Refl::repr{} : this->refl.constant_value.has_value() ? this->refl.constant_value.value() : static_cast<typename Refl::repr>(yyjson_get_bool(this->reference));
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Vector2> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				if constexpr (arrayVectors) {
					typename Refl::repr res{};

					assert(yyjson_arr_size(this->reference) == 2);
					size_t i, max;
					yyjson_val* item;
					yyjson_arr_foreach(this->reference, i, max, item) {
						res(i, 0) = yyjson_get_num(item);
					}

					return res;
				}
				else {
					return {
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "x"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "y"))),
					};
				}
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Vector3> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				if constexpr (arrayVectors) {
					typename Refl::repr res{};

					assert(yyjson_arr_size(this->reference) == 3);
					size_t i, max;
					yyjson_val* item;
					yyjson_arr_foreach(this->reference, i, max, item) {
						res(i, 0) = yyjson_get_num(item);
					}

					return res;
				}
				else {
					return {
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "x"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "y"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "z"))),
					};
				}
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Vector4> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				if constexpr (arrayVectors) {
					typename Refl::repr res{};

					assert(yyjson_arr_size(this->reference) == 4);
					size_t i, max;
					yyjson_val* item;
					yyjson_arr_foreach(this->reference, i, max, item) {
						res(i, 0) = yyjson_get_num(item);
					}

					return res;
				}
				else {
					return {
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "x"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "y"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "z"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "w"))),
					};
				}
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Quaternion> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				if constexpr (arrayVectors) {
					typename Refl::repr res{};

					assert(yyjson_arr_size(this->reference) == 4);
					size_t i, max;
					yyjson_val* item;
					yyjson_arr_foreach(this->reference, i, max, item) {
						res.coeffs()(i, 0) = yyjson_get_num(item);
					}

					return res;
				}
				else {
					return {
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "w"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "x"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "y"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "z"))),
					};
				}
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Position> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				if constexpr (arrayVectors) {
					typename Refl::repr res{};

					assert(yyjson_arr_size(this->reference) == 3);
					size_t i, max;
					yyjson_val* item;
					yyjson_arr_foreach(this->reference, i, max, item) {
						res(i, 0) = yyjson_get_num(item);
					}

					return res;
				}
				else {
					return {
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "x"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "y"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "z"))),
					};
				}
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Rotation> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				if constexpr (arrayVectors) {
					typename Refl::repr res{};

					assert(yyjson_arr_size(this->reference) == 4);
					size_t i, max;
					yyjson_val* item;
					yyjson_arr_foreach(this->reference, i, max, item) {
						res.coeffs()(i, 0) = yyjson_get_num(item);
					}

					return res;
				}
				else {
					return {
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "w"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "x"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "y"))),
						static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "z"))),
					};
				}
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Matrix34> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				typename Refl::repr res{};

				assert(yyjson_arr_size(this->reference) == 12);
				size_t i, max;
				yyjson_val* item;
				yyjson_arr_foreach(this->reference, i, max, item) {
					res(i / 4, i % 4) = yyjson_get_num(item);
				}

				return res;
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::math::Matrix44> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				typename Refl::repr res{};

				assert(yyjson_arr_size(this->reference) == 16);
				size_t i, max;
				yyjson_val* item;
				yyjson_arr_foreach(this->reference, i, max, item) {
					res(i / 4, i % 4) = yyjson_get_num(item);
				}

				return res;
			}
		};
		template<typename Refl, ucsl::colors::ChannelOrder order>
		class PrimitiveDataAccessor<Refl, ucsl::colors::Color8<order>> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				typename Refl::repr res{};
				
				res.r = static_cast<uint8_t>(yyjson_get_uint(yyjson_obj_get(this->reference, "r")));
				res.g = static_cast<uint8_t>(yyjson_get_uint(yyjson_obj_get(this->reference, "g")));
				res.b = static_cast<uint8_t>(yyjson_get_uint(yyjson_obj_get(this->reference, "b")));
				res.a = static_cast<uint8_t>(yyjson_get_uint(yyjson_obj_get(this->reference, "a")));

				return res;
			}
		};
		template<typename Refl, ucsl::colors::ChannelOrder order>
		class PrimitiveDataAccessor<Refl, ucsl::colors::Colorf<order>> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				typename Refl::repr res{};
				
				res.r = static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "r")));
				res.g = static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "g")));
				res.b = static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "b")));
				res.a = static_cast<float>(yyjson_get_num(yyjson_obj_get(this->reference, "a")));

				return res;
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::objectids::ObjectIdV1> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				return util::fromGUID<typename Refl::repr>(yyjson_get_str(this->reference));
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::objectids::ObjectIdV2> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				return util::fromGUID<typename Refl::repr>(yyjson_get_str(this->reference));
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, ucsl::strings::VariableString> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				return yyjson_get_str(this->reference);
			}
		};
		template<typename Refl>
		class PrimitiveDataAccessor<Refl, const char*> : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator typename Refl::repr () const {
				return yyjson_get_str(this->reference);
			}
		};

		template<typename Refl>
		class PrimitiveAccessor : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			template<typename F>
			inline const auto visit(F f) const {
				return this->refl.visit([&](auto r){ return f(PrimitiveDataAccessor<decltype(r)>{ this->reference, r }); });
			}
		};

		template<typename Refl>
		class EnumAccessor : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			operator long long () const {
				if (yyjson_is_int(this->reference)) {
					return yyjson_get_sint(this->reference);
				}
				if (yyjson_is_str(this->reference)) {
					const char* str = yyjson_get_str(this->reference);

					for (auto& option : this->refl.get_options())
						if (!strcmp(option.GetEnglishName(), str))
							return option.GetIndex();
				}
				assert("unhandled enum");
				return -1;
			}
		};

		template<typename Refl>
		class StructureAccessor : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			template<typename FieldRefl>
			inline const auto operator[](const FieldRefl& field_refl) const {
				auto type = field_refl.get_type(*this);

				return ValueAccessor<decltype(type)>{ Reference{ this->reference, field_refl.get_name() }, type };
			}

			inline auto get_base() const {
				auto base = this->refl.get_base();

				return base.has_value() ? std::make_optional(StructureAccessor<std::remove_reference_t<decltype(base.value())>>{ this->reference, base.value() }) : std::nullopt;
			}
		};

		template<typename Refl>
		class CArrayAccessor : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			inline size_t get_length() const {
				return this->refl.get_length();
			}

			inline const auto operator[](size_t idx) const {
				auto item_refl = this->refl.get_item_type();

				assert(idx < this->refl.get_length());

				return ValueAccessor<decltype(item_refl)>{ { this->reference, idx }, item_refl };
			}
		};

		template<typename Refl>
		class PointerAccessor : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			inline auto get() const {
				auto target_type = this->refl.get_target_type();

				return yyjson_is_null(this->reference) ? std::nullopt : std::make_optional<const ValueAccessor<decltype(target_type)>>({ this->reference, target_type });
			}
		};

		template<typename Refl>
		class ArrayAccessor : public Accessor<Refl> {
		public:
			class const_iterator {
				ArrayAccessor& accessor;
				yyjson_val* cur;
				size_t idx{};

			public:
				const_iterator(ArrayAccessor& accessor, yyjson_val* cur, size_t idx) : accessor{ accessor }, cur{ cur }, idx{ idx } {}
				const_iterator(const const_iterator& other) : accessor{ other.accessor }, cur{ other.cur }, idx{ other.idx } {}

				const_iterator& operator++() {
					cur = unsafe_yyjson_get_next(cur);
					idx++;
					return *this;
				}

				const_iterator operator++(int) {
					const_iterator result{ *this };
					cur = unsafe_yyjson_get_next(cur);
					idx++;
					return result;
				}

				bool operator==(const const_iterator& other) const { return idx == other.idx; }
				bool operator!=(const const_iterator& other) const { return idx != other.idx; }
				bool operator<(const const_iterator& other) const { return idx < other.idx; }
				bool operator>(const const_iterator& other) const { return idx > other.idx; }
				bool operator<=(const const_iterator& other) const { return idx <= other.idx; }
				bool operator>=(const const_iterator& other) const { return idx >= other.idx; }
				const auto operator*() const {
					auto item_type = accessor.refl.get_item_type();

					return ValueAccessor<decltype(item_type)>{ { accessor.reference, idx, cur }, item_type };
				}
			};

			using Accessor<Refl>::Accessor;

			const_iterator begin() const { return { *this, yyjson_arr_get_first(this->reference), 0 }; }
			const_iterator cbegin() const { return { *this, yyjson_arr_get_first(this->reference), 0 }; }
			const_iterator end() const { return { *this, nullptr, size() }; }
			const_iterator cend() const { return { *this, nullptr, size() }; }

			const auto operator[](size_t i) const {
				auto item_type = this->refl.get_item_type();

				return ValueAccessor<decltype(item_type)>{ { this->reference, i }, item_type };
			}

			size_t size() const { return yyjson_arr_size(this->reference); }
			size_t capacity() const { return yyjson_arr_size(this->reference); }
		};

		template<typename Refl>
		class ValueAccessor : public Accessor<Refl> {
		public:
			using Accessor<Refl>::Accessor;

			const auto visit(auto f) const {
				return this->refl.visit([&](auto r) {
					if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::PRIMITIVE) return f(PrimitiveAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ENUM) return f(EnumAccessor<decltype(r)>{ this->reference, r });
					//else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::FLAGS) return f(flags(this->reference, refl.is_erased(), r));
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ARRAY) return f(ArrayAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::TARRAY) return f(ArrayAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::POINTER) return f(PointerAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::CARRAY) return f(CArrayAccessor<decltype(r)>{ this->reference, r });
					//else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::UNION) return f(union(this->reference, parent, r));
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::STRUCTURE) return f(StructureAccessor<decltype(r)>{ this->reference, r });
					else static_assert(false, "invalid type kind");
				});
			}

			const auto as_primitive() const {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::PRIMITIVE) return PrimitiveAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a primitive"); });
			}

			const auto as_enum() const {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ENUM) return EnumAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not an enum"); });
			}

			const auto as_array() const {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ARRAY) return ArrayAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a array"); });
			}

			const auto as_tarray() const {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::TARRAY) return ArrayAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a tarray"); });
			}

			const auto as_carray() const {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::CARRAY) return CArrayAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a carray"); });
			}

			const auto as_pointer() const {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::POINTER) return PointerAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a pointer"); });
			}

			const auto as_structure() const {
				return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::STRUCTURE) return StructureAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a structure"); });
			}
		};
	};
}