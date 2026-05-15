#pragma once
#include <type_traits>
#include <ucsl-reflection/reflections/basic-types.h>
#include <ucsl-reflection/traversals/types.h>
#include <ucsl-reflection/accessors/types.h>
#include <ucsl-reflection/opaque.h>
#include <rip/binary/types.h>
#include "BlobWorker.h"
#include <iostream>

namespace rip::binary {
	using namespace ucsl::reflection;
	using namespace ucsl::reflection::traversals;

	template<typename Backend>
	class BinarySerializer {
		Backend& backend;

		//// We keep the buffer size as well, since sometimes an earlier reference serialized a smaller slice
		//// of the buffer, and in that case we can't simply point back to this already stored version.
		//// We could do a lot of work to roll back and instead store a larger buffer, but for now I'm just
		//// duplicating the data in that case, since it mostly happens with dangling pointers of empty MoveArrays.
		//struct DisambiguatedPointer {
		//	size_t offset;
		//	size_t bufferSize;
		//};

		//std::map<const void*, DisambiguatedPointer> knownPtrs{};
		BlobWorker<> worker{ backend.tellp() };

		template<typename T, typename F>
		offset_t<T> enqueueBlock(size_t bufferSize, size_t alignment, F processFunc) {
			//if (ptr == nullptr)
			//	return offset_t<T>{};

			//auto it = knownPtrs.find(ptr);
			//if (it != knownPtrs.end() && bufferSize <= it->second.bufferSize)
			//	return it->second.offset;

			size_t offset = worker.enqueueBlock(bufferSize, alignment, [this, bufferSize, processFunc](size_t offset, size_t alignment) {
				backend.write_padding(alignment);
				assert(backend.tellp() == offset);

				processFunc();

				assert(backend.tellp() == offset + bufferSize);
			});

			//knownPtrs[ptr] = { offset, bufferSize };
			return offset;
		}

		template<typename T, std::enable_if_t<!std::is_fundamental_v<T>, bool> = true>
		inline void process_primitive_data(const T& obj, bool erased) {
			backend.template write<T>(obj);
		}

		template<typename T, std::enable_if_t<std::is_fundamental_v<T>, bool> = true>
		inline void process_primitive_data(const T& obj, bool erased) {
			backend.write(erased ? T{} : obj);
		}

		inline void write_string(const char* obj) {
			if constexpr (Backend::hasNativeStrings)
				backend.write(obj);
			else
				backend.write(obj == nullptr ? offset_t<char>{} : enqueueBlock<char>(strlen(obj) + 1, 1, [this, obj]() {
					backend.write_string(obj);
				}));
		}

		inline void write_string(const std::string& obj) {
			if constexpr (Backend::hasNativeStrings)
				backend.write(obj.c_str());
			else
				backend.write(enqueueBlock<char>(obj.size() + 1, 1, [this, obj]() {
					backend.write_string(obj.c_str());
				}));
		}

		inline void process_primitive_data(ucsl::strings::VariableString& obj, bool erased) {
			write_string(reinterpret_cast<const char*&>(obj));
			backend.write(0ull);
		}

		inline void process_primitive_data(void*& obj, bool erased) {
			if (obj == nullptr)
				backend.write(offset_t<opaque_obj>{});
			//else if (knownPtrs.contains(obj))
			//	backend.write(offset_t<opaque_obj>{ knownPtrs[obj].offset });
			else {
				//for (auto& item : knownPtrs) {
				//	if (obj >= item.first && obj < addptr(item.first, item.second.bufferSize)) {
				//		backend.write(offset_t<opaque_obj>{ item.second.offset + (reinterpret_cast<size_t>(obj) - reinterpret_cast<size_t>(item.first)) });
				//	}
				//}
				assert(false && "cannot find backreference");
			}
		}

		template<ucsl::reflection::accessors::PrimitiveAccessor T>
		inline void process_primitive(const T& obj) {
			obj.visit([&](auto data) {
				if constexpr (std::is_same_v<typename decltype(data.refl)::repr, const char*>) {
					write_string(std::string{ data });
				}
				else
					process_primitive_data(typename decltype(data.refl)::repr{ data }, data.refl.is_erased);
			});
		}

		template<typename T>
		inline void process_enum(const T& obj) {
			obj.refl.visit([&](auto refl) {
				process_primitive_data(static_cast<decltype(refl)::repr>(static_cast<long long>(obj)), refl.is_erased);
			});
		}

		//template<typename T>
		//inline void process_flags(const T& obj) {
		//	process_primitive(obj);
		//}

		template<typename T>
		inline void process_array(const T& arr) {
			auto itemRefl = arr.refl.get_item_type();

			backend.write(arr.size() == 0 ? offset_t<void>{} : enqueueBlock<void>(arr.size() * itemRefl.get_size(arr), itemRefl.get_alignment(), [this, arr]() {
				for (const auto& item : arr)
					process_type(item);
			}));
			backend.write(arr.size());
			backend.write(arr.capacity());
			backend.write(0ull);
		}

		template<typename T>
		inline void process_tarray(const T& arr) {
			auto itemRefl = arr.refl.get_item_type();

			backend.write(arr.size() == 0 ? offset_t<void>{} : enqueueBlock<void>(arr.size() * itemRefl.get_size(arr), itemRefl.get_alignment(), [this, arr]() {
				for (const auto& item : arr)
					process_type(item);
			}));
			backend.write(arr.size());
			backend.write(static_cast<int64_t>(arr.capacity()));
		}

		template<ucsl::reflection::accessors::CArrayAccessor T>
		inline void process_carray(const T& arr) {
			for (const auto& item : arr)
				process_type(item);
		}

		template<ucsl::reflection::accessors::PointerAccessor T>
		inline void process_pointer(const T& obj) {
			auto targetRefl = obj.refl.get_target_type();

			backend.write(!obj.get().has_value() ? offset_t<void>{} : enqueueBlock<void>(targetRefl.get_size(obj.get().value()), targetRefl.get_alignment(), [this, obj]() {
				process_type(obj.get().value());
			}));
		}

		template<typename T>
		inline void process_union(const T& obj) {
			obj.visit([&](auto field) { process_type(field); });
		}

		template<ucsl::reflection::accessors::ValueAccessor T>
		inline void process_type(const T& obj) {
			backend.write_padding(obj.refl.get_alignment());

			size_t typeStart = backend.tellp();

			obj.visit([&](auto v) {
				if constexpr (decltype(v.refl)::kind == providers::TypeKind::PRIMITIVE) process_primitive(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::ENUM) process_enum(v);
				//else if constexpr (decltype(v.refl)::kind == providers::TypeKind::FLAGS) process_flags(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::ARRAY) process_array(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::TARRAY) process_tarray(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::CARRAY) process_carray(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::POINTER) process_pointer(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::UNION) process_union(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::STRUCTURE) process_struct(v);
				else static_assert(false, "invalid type kind");
			});

			backend.write_padding_bytes(obj.refl.get_size(obj) - (backend.tellp() - typeStart));
		}

		template<ucsl::reflection::accessors::StructureAccessor T>
		inline void process_fields(const T& obj) {
			auto base = obj.get_base();

			if (base.has_value())
				process_fields(base.value());

			obj.refl.visit_fields(obj, [&](auto field) {
				process_type(obj[field]);
			});
		}

		template<ucsl::reflection::accessors::StructureAccessor T>
		inline void process_struct(const T& obj) {
			process_fields(obj);
		}

	public:
		BinarySerializer(Backend& backend) : backend{ backend } {}

		template<typename T>
		inline void process(const T& obj) {
			enqueueBlock<void>(obj.refl.get_size(obj), obj.refl.get_alignment(), [this, obj]() { process_type(obj); });
			worker.processQueuedBlocks();
		}
	};

	template<typename Backend>
	void serializeBinaryToBinaryStream(Backend& backend, const auto& acc) {
		BinarySerializer serializer{ backend };
		serializer.process(acc);
	}

	template<typename Backend, typename AddrType, std::endian endianness = std::endian::native, bool byteswap_offsets = true, bool relative_offsets = false>
	void serializeBinaryToStream(Backend& backend, const auto& acc) {
		binary_ostream<Backend, AddrType, endianness, byteswap_offsets, relative_offsets> bos{ backend };

		serializeBinaryToBinaryStream(bos, acc);
	}

	template<typename AddrType, std::endian endianness = std::endian::native, bool byteswap_offsets = true, bool relative_offsets = false>
	void serializeBinaryToBuffer(void* buf, const auto& acc) {
		mem_ostream mos{ buf };

		serializeBinaryToStream<mem_ostream, AddrType, endianness, byteswap_offsets, relative_offsets>(mos, acc);
	}

	template<typename AddrType, std::endian endianness = std::endian::native, bool byteswap_offsets = true, bool relative_offsets = false>
	size_t measureBinary(const auto& acc) {
		null_ostream nos{};

		serializeBinaryToStream<null_ostream, AddrType, endianness, byteswap_offsets, relative_offsets>(nos, acc);

		return nos.tellp();
	}

	template<typename AllocatorSystem, typename AddrType, std::endian endianness = std::endian::native, bool byteswap_offsets = true, bool relative_offsets = false>
	void* serializeBinaryToAllocatorSystemBuffer(const auto& acc) {
		void* buf = new (AllocatorSystem::get_allocator()) uint8_t[measureBinary<AddrType, endianness, byteswap_offsets, relative_offsets>(acc)];

		serializeBinaryToBuffer<AddrType, endianness, byteswap_offsets, relative_offsets>(buf, acc);

		return buf;
	}

	template<typename AddrType, std::endian endianness = std::endian::native, bool byteswap_offsets = true, bool relative_offsets = false>
	void* serializeBinaryToNativeBuffer(const auto& acc) {
		void* buf = new uint8_t[measureBinary<AddrType, endianness, byteswap_offsets, relative_offsets>(acc)];

		serializeBinaryToBuffer<AddrType, endianness, byteswap_offsets, relative_offsets>(buf, acc);

		return buf;
	}
}