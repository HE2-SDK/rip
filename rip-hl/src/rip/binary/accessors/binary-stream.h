#pragma once
#include <ucsl/containers/arrays/array.h>
#include <ucsl/containers/arrays/tarray.h>
#include <ucsl-reflection/providers/types.h>
#include <ucsl-reflection/util/memory.h>
#include <rip/binary/stream.h>

namespace rip::binary::accessors {
	template<typename Stream>
	struct binary_istream {
		struct opaque_value {};

		struct Reference {
			Stream& stream;
			size_t offset;

			constexpr Reference(Stream& stream, size_t offset) : stream{ stream }, offset{ offset } {}
			constexpr Reference(Stream& stream) : stream{ stream }, offset{ stream.tellg() } {}

			inline auto withStream(auto f) const {
				auto prevOff = stream.tellg();
				stream.seekg(offset);
				auto res = f(stream);
				stream.seekg(prevOff);
				return res;
			}

			bool operator==(const Reference& other) const {
				return &stream == &other.stream && offset == other.offset;
			}

			bool operator!=(const Reference& other) const {
				return &stream != &other.stream || offset != other.offset;
			}
		};

		template<typename Refl>
		class AddressAccessor;

		template<typename Refl>
		class AccessorBase {
		public:
			const Reference reference;
			const Refl refl;

			constexpr AccessorBase(const Reference& reference, const Refl& refl) : reference{ reference }, refl{ refl } {}

			constexpr AddressAccessor<Refl> operator&() const {
				return { reference, refl };
			}
		};

		template<typename Refl>
		class ValueAccessor;

		template<typename Refl>
		class PrimitiveDataAccessor : public AccessorBase<Refl> {
		public:
			using AccessorBase<Refl>::AccessorBase;

			inline operator std::conditional_t<std::is_same_v<typename Refl::repr, const char*> || std::is_same_v<typename Refl::repr, ucsl::strings::VariableString>,std::string,typename Refl::repr>() const {
				if constexpr (std::is_same_v<typename Refl::repr, const char*> || std::is_same_v<typename Refl::repr, ucsl::strings::VariableString>) {
					return this->reference.withStream([&](auto& stream) {
						rip::binary::offset_t<const char> offset{};
						stream.read(offset);

						std::string res{};

						if (offset.has_value()) {
							auto pos = stream.tellg();
							stream.seekg(offset.value());
							stream.read_string(res);
							stream.seekg(pos);
						}

						return res;
					});
				}
				else {
					return this->reference.withStream([&](auto& stream) {
						typename Refl::repr res;
						stream.read(res);
						return res;
					});
				}
			}
		};

		template<typename Refl>
		class PrimitiveAccessor : public AccessorBase<Refl> {
		public:
			using AccessorBase<Refl>::AccessorBase;

			template<typename F>
			constexpr const auto visit(F f) const {
				return this->refl.visit([&](auto r) { return f(PrimitiveDataAccessor<decltype(r)>{ this->reference, r }); });
			}

			template<typename T>
			constexpr const auto as() const {
				return this->refl.visit([&](auto r) { if constexpr (std::is_same_v<typename decltype(r)::repr, T>) return PrimitiveDataAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not the correct primitive type"); });
			}
		};

		template<typename Refl>
		class EnumAccessor : public AccessorBase<Refl> {
		public:
			using AccessorBase<Refl>::AccessorBase;
			
			operator long long () const {
				return this->refl.visit([&](auto r){
					PrimitiveDataAccessor<decltype(r)> pd{ this->reference, r };

					return static_cast<long long>(pd);
				});
			}

			//template<typename F>
			//constexpr const auto visit(F f) const {
			//	return this->refl.visit([&](auto r) { return f(PrimitiveDataAccessor<decltype(r)>{ this->reference, r }); });
			//}

			//template<typename T>
			//constexpr const auto as() const {
			//	return this->refl.visit([&](auto r) { if constexpr (std::is_same_v<typename decltype(r)::underlying, T>) return PrimitiveDataAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not the correct primitive type"); });
			//}
		};

		template<typename Refl>
		class BitfieldAccessor : public AccessorBase<Refl> {
		public:
			using AccessorBase<Refl>::AccessorBase;

			template<typename F>
			constexpr const auto visit(F f) const {
				return this->refl.visit_underlying([&](auto r) { return f(PrimitiveDataAccessor<decltype(r)>{ this->reference, r }); });
			}

			template<typename T>
			constexpr const auto as() const {
				return this->refl.visit_underlying([&](auto r) { if constexpr (std::is_same_v<typename decltype(r)::repr, T>) return PrimitiveDataAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not the correct primitive type"); });
			}
		};

		template<typename Refl>
		class StructureAccessor : public AccessorBase<Refl> {
		public:
			using AccessorBase<Refl>::AccessorBase;

			template<typename FieldRefl>
			constexpr auto operator[](const FieldRefl& field_refl) const {
				auto type = field_refl.get_type(*this);

				return ValueAccessor<decltype(type)>{ { this->reference.stream, this->reference.offset + field_refl.get_offset() }, type };
			}

			template<simplerfl::strlit FieldName>
			constexpr auto get_field() const {
				return (*this)[this->refl.template get_field<FieldName, typename Stream::AddrType>(*this)];
			}

			constexpr void visit_fields(auto f) const {
				return this->refl.template visit_fields<typename Stream::AddrType>(*this, [&](const auto& field) { f((*this)[field], field); });
			}

			constexpr auto get_base() const {
				auto base = this->refl.get_base();

				return base.has_value() ? std::make_optional(StructureAccessor<std::remove_reference_t<decltype(base.value())>>{ this->reference, base.value() }) : std::nullopt;
			}
		};

		template<typename Refl>
		class CArrayAccessor : public AccessorBase<Refl> {
		public:
			using AccessorBase<Refl>::AccessorBase;

			class const_iterator {
				const CArrayAccessor& accessor;
				size_t idx{};

			public:
				inline const_iterator(const CArrayAccessor& accessor, size_t idx) : accessor{ accessor }, idx{ idx } {}
				inline const_iterator(const const_iterator& other) : accessor{ other.accessor }, idx{ other.idx } {}

				inline const_iterator& operator++() {
					idx++;
					return *this;
				}

				inline const_iterator operator++(int) {
					const_iterator result{ *this };
					idx++;
					return result;
				}

				inline const_iterator& operator--() {
					idx--;
					return *this;
				}

				inline const_iterator operator--(int) {
					const_iterator result{ *this };
					idx--;
					return result;
				}

				inline bool operator==(const const_iterator& other) const { return idx == other.idx; }
				inline bool operator!=(const const_iterator& other) const { return idx != other.idx; }
				inline bool operator<(const const_iterator& other) const { return idx < other.idx; }
				inline bool operator>(const const_iterator& other) const { return idx > other.idx; }
				inline bool operator<=(const const_iterator& other) const { return idx <= other.idx; }
				inline bool operator>=(const const_iterator& other) const { return idx >= other.idx; }
				inline const auto operator*() const { return accessor[idx]; }
			};

			constexpr size_t size() const {
				return this->refl.get_length();
			}

			constexpr auto operator[](size_t idx) const {
				auto item_refl = this->refl.get_item_type();

				//assert(idx < this->refl.get_length());

				return ValueAccessor<decltype(item_refl)>{ { this->reference.stream, this->reference.offset + idx * item_refl.template get_size<typename Stream::AddrType>(ValueAccessor<decltype(item_refl)>{ { this->reference.stream, this->reference.offset }, item_refl }) }, item_refl };
			}

			inline const_iterator begin() const { return { *this, 0 }; }
			inline const_iterator cbegin() const { return { *this, 0 }; }
			inline const_iterator end() const { return { *this, size() }; }
			inline const_iterator cend() const { return { *this, size() }; }
		};

		template<typename Refl>
		class AddressAccessor : public AccessorBase<Refl> {
		public:
			using AccessorBase<Refl>::AccessorBase;

			constexpr operator bool() const {
				return this->reference.offset != 0;
			}

			constexpr operator size_t() const {
				return this->reference.offset;
			}

			constexpr ValueAccessor<Refl> operator*() const noexcept {
				return { this->reference, this->refl };
			}

			//constexpr auto operator->() const noexcept {
			//	return ValueAccessor<decltype(target_type)>{ this->reference, this->refl };
			//}

			constexpr bool operator==(std::nullptr_t ptr) const {
				return this->reference.offset == 0;
			}

			constexpr bool operator==(const AddressAccessor<Refl>& other) const {
				return this->reference == other->reference;
			}

			constexpr bool operator!=(std::nullptr_t ptr) const {
				return !this->reference.offset != 0;
			}

			constexpr bool operator!=(const AddressAccessor<Refl>& other) const {
				return this->reference != other->reference;
			}
		};

		template<typename Refl>
		class PointerAccessor : public AccessorBase<Refl> {
		public:
			using AccessorBase<Refl>::AccessorBase;

			//inline auto get() const {
			//	auto target_type = this->refl.get_target_type();

			//	return this->reference.withStream([&](auto& stream) {
			//		rip::binary::offset_t<void> offset{};
			//		stream.read(offset);

			//		return !offset.has_value() ? std::nullopt : std::make_optional<const ValueAccessor<decltype(target_type)>>({ { this->reference.stream, offset.value() }, target_type });
			//	});
			//}

			constexpr auto get() const {
				return this->reference.withStream([&](auto& stream) {
					auto target_type = this->refl.get_target_type();

					rip::binary::offset_t<void> offset{};
					stream.read(offset);

					if (!offset.has_value())
						return AddressAccessor<decltype(target_type)> { { this->reference.stream, 0ull }, target_type };

					AddressAccessor<decltype(target_type)> addr{ { this->reference.stream, offset.value() }, target_type };

					return addr;// target_type.template get_size<typename Stream::AddrType>(*addr) > 0 ? addr : AddressAccessor<decltype(target_type)>{ {this->reference.stream, 0ull}, target_type };
				});
			}

			constexpr operator auto() const {
				return get();
			}

			constexpr operator size_t() const {
				return get();
			}

			constexpr operator bool() const {
				return get();
			}

			constexpr auto operator*() const noexcept {
				return *get();
			}

			//constexpr auto operator->() const noexcept {
			//	return *get();
			//}

			constexpr bool operator==(std::nullptr_t ptr) const {
				return get() == ptr;
			}

			constexpr bool operator==(const PointerAccessor<Refl>& other) const {
				return get() == other.get();
			}

			constexpr bool operator!=(std::nullptr_t ptr) const {
				return get() != ptr;
			}

			constexpr bool operator!=(const PointerAccessor<Refl>& other) const {
				return get() != other.get();
			}
		};

		template<typename Refl>
		class UnionAccessor : public AccessorBase<Refl> {
		public:
			using AccessorBase<Refl>::AccessorBase;

			template<typename FieldRefl>
			constexpr auto operator[](const FieldRefl& field_refl) const {
				auto type = field_refl.get_type(*this);

				return ValueAccessor<decltype(type)>{ this->reference, type };
			}

			template<simplerfl::strlit FieldName>
			constexpr auto get_field() const {
				return (*this)[this->refl.get_field<FieldName>(*this)];
			}

			template<typename F>
			constexpr void visit(F f) const {
				this->refl.visit_current_field([&](const auto& field_refl) {
					f((*this)[field_refl]);
				});
			}
		};

		template<template<typename, typename> typename A, typename Refl>
		class ArrayAccessor : public AccessorBase<Refl> {
		public:
			class const_iterator {
				const ArrayAccessor& accessor;
				size_t idx{};

			public:
				inline const_iterator(const ArrayAccessor& accessor, size_t idx) : accessor{ accessor }, idx{ idx } {}
				inline const_iterator(const const_iterator& other) : accessor{ other.accessor }, idx{ other.idx } {}

				inline const_iterator& operator++() {
					idx++;
					return *this;
				}

				inline const_iterator operator++(int) {
					const_iterator result{ *this };
					idx++;
					return result;
				}

				inline const_iterator& operator--() {
					idx--;
					return *this;
				}

				inline const_iterator operator--(int) {
					const_iterator result{ *this };
					idx--;
					return result;
				}

				inline bool operator==(const const_iterator& other) const { return idx == other.idx; }
				inline bool operator!=(const const_iterator& other) const { return idx != other.idx; }
				inline bool operator<(const const_iterator& other) const { return idx < other.idx; }
				inline bool operator>(const const_iterator& other) const { return idx > other.idx; }
				inline bool operator<=(const const_iterator& other) const { return idx <= other.idx; }
				inline bool operator>=(const const_iterator& other) const { return idx >= other.idx; }
				inline const auto operator*() const { return accessor[idx]; }
			};

			//template<typename T> OpaqueReflArray(A<T, typename GameInterface::AllocatorSystem>& underlying, Refl refl) : underlying{ static_cast<RflArray<A>&>(underlying) }, refl{ refl } {}
			//template<typename T> OpaqueReflArray(const OpaqueReflArray<A, GameInterface, Refl>& other, Refl refl) : underlying{ other.underlying }, refl{ other.refl } {}
			
			using AccessorBase<Refl>::AccessorBase;

			inline const_iterator begin() const { return { *this, 0 }; }
			inline const_iterator cbegin() const { return { *this, 0 }; }
			inline const_iterator end() const { return { *this, size() }; }
			inline const_iterator cend() const { return { *this, size() }; }

			inline const auto operator[](size_t i) const {
				return this->reference.withStream([&](auto& stream) {
					auto item_type = this->refl.get_item_type();

					rip::binary::offset_t<void> offset{};

					stream.read(offset);

					return ValueAccessor<decltype(item_type)>{ { this->reference.stream, offset.value() + i * item_type.template get_size<typename Stream::AddrType>(ValueAccessor<decltype(item_type)>{ { this->reference.stream, offset.value() }, item_type }) }, item_type };
				});
			}

			inline size_t size() const {
				return this->reference.withStream([&](auto& stream) {
					auto item_type = this->refl.get_item_type();

					rip::binary::offset_t<void> offset{};
					rip::binary::size_val_t length{};

					stream.read(offset);
					stream.read(length);

					return static_cast<size_t>(length);
				});
			}

			inline size_t capacity() const {
				return this->reference.withStream([&](auto& stream) {
					auto item_type = this->refl.get_item_type();

					rip::binary::offset_t<void> offset{};
					rip::binary::size_val_t length{};
					rip::binary::size_val_t capacity{};

					stream.read(offset);
					stream.read(length);
					stream.read(capacity);

					return static_cast<size_t>(capacity);
				});
			}
		};

		template<typename Refl>
		class ValueAccessor : public AccessorBase<Refl> {
		public:
			using AccessorBase<Refl>::AccessorBase;

			constexpr const auto visit(auto f) const {
				return this->refl.visit([&](auto r) {
					if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::PRIMITIVE) return f(PrimitiveAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ENUM) return f(EnumAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::BITFIELD) return f(BitfieldAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ARRAY) return f(ArrayAccessor<ucsl::containers::arrays::Array, decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::TARRAY) return f(ArrayAccessor<ucsl::containers::arrays::TArray, decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::POINTER) return f(PointerAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::CARRAY) return f(CArrayAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::UNION) return f(UnionAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::STRUCTURE) return f(StructureAccessor<decltype(r)>{ this->reference, r });
					else static_assert(false, "invalid type kind");
				});
			}

			constexpr auto visit(auto f) {
				return this->refl.visit([&](auto r) {
					if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::PRIMITIVE) return f(PrimitiveAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ENUM) return f(EnumAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::BITFIELD) return f(BitfieldAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ARRAY) return f(ArrayAccessor<ucsl::containers::arrays::Array, decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::TARRAY) return f(ArrayAccessor<ucsl::containers::arrays::TArray, decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::POINTER) return f(PointerAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::CARRAY) return f(CArrayAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::UNION) return f(UnionAccessor<decltype(r)>{ this->reference, r });
					else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::STRUCTURE) return f(StructureAccessor<decltype(r)>{ this->reference, r });
					else static_assert(false, "invalid type kind");
				});
			}

			constexpr auto as_primitive() const { return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::PRIMITIVE) return PrimitiveAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a primitive"); }); }
			constexpr auto as_enum() const { return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ENUM) return EnumAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not an enum"); }); }
			constexpr auto as_bitfield() const { return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::BITFIELD) return BitfieldAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a bitfield"); }); }
			constexpr auto as_array() const { return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::ARRAY) return ArrayAccessor<ucsl::containers::arrays::Array, decltype(r)>{ this->reference, r }; else static_assert(false, "not a array"); }); }
			constexpr auto as_tarray() const { return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::TARRAY) return ArrayAccessor<ucsl::containers::arrays::TArray, decltype(r)>{ this->reference, r }; else static_assert(false, "not a tarray"); }); }
			constexpr auto as_carray() const { return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::CARRAY) return CArrayAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a carray"); }); }
			constexpr auto as_pointer() const { return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::POINTER) return PointerAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a pointer"); }); }
			constexpr auto as_union() const { return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::UNION) return UnionAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a union"); }); }
			constexpr auto as_structure() const { return this->refl.visit([&](auto r) { if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::STRUCTURE) return StructureAccessor<decltype(r)>{ this->reference, r }; else static_assert(false, "not a structure"); }); }
		};
	};
}
