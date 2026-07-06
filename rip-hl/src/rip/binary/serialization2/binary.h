#pragma once
#include <type_traits>
#include <ucsl-reflection/reflections/basic-types.h>
#include <ucsl-reflection/traversals/types.h>
#include <ucsl-reflection/accessors/types.h>
#include <ucsl-reflection/opaque.h>
#include <rip/binary/types.h>
//#include "BlobWorker.h"
#include <iostream>
#include <queue>

namespace rip::binary {
	using namespace ucsl::reflection;
	using namespace ucsl::reflection::traversals;

	template<typename Backend>
	class BinarySerializer {
		Backend& backend;

		struct DisambiguatedPointer {
			std::optional<offset_t<void>> resolvedTarget{};
			std::vector<size_t> weakPtrs{};
		};

		struct WorkQueueEntry {
			size_t offset;
			size_t alignment;
			std::function<void()> processFunc;
		};

		std::unordered_map<size_t, DisambiguatedPointer> knownPtrs{};
		std::queue<WorkQueueEntry> workQueue{};
		size_t nextOffset{};

		inline size_t allocate(size_t size, size_t alignment) {
			size_t offset = align(nextOffset, alignment);
			nextOffset = offset + size;
			return offset;
		}

		template<typename Refl>
		constexpr size_t countPtrDepth(const Refl& refl) {
			return refl.visit([&](auto r) {
				if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::POINTER) { return 1 + countPtrDepth(r.get_target_type()); }
				else if constexpr (decltype(r)::kind == ucsl::reflection::providers::TypeKind::CARRAY) { return 1 + countPtrDepth(r.get_item_type()); }
				else return 0;
			});
		}

		template<typename T>
		inline DisambiguatedPointer& lookupPtr(const T& obj) {
			return knownPtrs[static_cast<size_t>(&obj) ^ countPtrDepth(obj.refl)];
		}

		inline void resolvePtr(DisambiguatedPointer& knownPtr, offset_t<void> targetOffset) {
			auto pos = backend.tellp();

			for (auto weakPtrLoc : knownPtr.weakPtrs) {
				backend.seekp(weakPtrLoc);
				backend.write(targetOffset);
			}

			knownPtr.weakPtrs.clear();
			knownPtr.resolvedTarget = targetOffset;

			backend.seekp(pos);
		}

		template<typename T>
		inline offset_t<T> enqueueBlock(size_t bufferSize, size_t alignment, auto processFunc) {
			assert(bufferSize > 0);

			//if (ptr == nullptr)
			//	return offset_t<T>{};

			//auto it = knownPtrs.find(ptr);
			//if (it != knownPtrs.end() && bufferSize <= it->second.bufferSize)
			//	return it->second.offset;

			size_t offset = allocate(bufferSize, alignment);
			workQueue.push(WorkQueueEntry{ offset, alignment, processFunc });
			return offset;
		}

		inline void processQueuedBlocks() {
			while (!workQueue.empty()) {
				auto chunk = workQueue.front();
				workQueue.pop();

				backend.write_padding(chunk.alignment);
				chunk.processFunc();
			}
		}

		template<typename T, std::enable_if_t<!std::is_fundamental_v<T>, bool> = true>
		inline void process_primitive_data(const T& obj, bool erased) {
			backend.template write<T>(obj);
		}

		template<typename T, std::enable_if_t<std::is_fundamental_v<T>, bool> = true>
		inline void process_primitive_data(const T& obj, bool erased) {
			backend.write(erased ? T{} : obj);
		}

		inline void write_string(const std::string& obj) {
			if constexpr (Backend::hasNativeStrings)
				backend.write(obj.c_str());
			else
				backend.write(enqueueBlock<char>(obj.size() + 1, 1, [this, obj]() {
					backend.write_string(obj.c_str());
				}));
		}

		template<ucsl::reflection::accessors::PrimitiveAccessor T>
		inline void process_primitive(const T& obj) {
			obj.visit([&](const auto& data) {
				if constexpr (std::is_same_v<typename decltype(data.refl)::repr, const char*>)
					write_string(std::string{ data });
				else if constexpr (std::is_same_v<typename decltype(data.refl)::repr, ucsl::strings::VariableString>) {
					write_string(std::string{ data });
					backend.write(size_val_t{ 0ull });
				}
				else {
					typename decltype(data.refl)::repr val{};
					val = static_cast<typename decltype(data.refl)::repr>(data);
					process_primitive_data(val, data.refl.is_erased);
				}
			});
		}

		template<typename T>
		inline void process_enum(const T& obj) {
			obj.refl.visit([&](const auto& refl) {
				process_primitive_data(static_cast<std::decay_t<decltype(refl)>::repr>(static_cast<long long>(obj)), refl.is_erased);
			});
		}

		template<typename T>
		inline void process_bitfield(const T& obj) {
			obj.visit([&](const auto& data) {
				process_primitive_data(static_cast<typename decltype(data.refl)::repr>(data), data.refl.is_erased);
			});
		}

		template<typename T>
		inline void process_array(const T& arr) {
			auto itemRefl = arr.refl.get_item_type();

			backend.write(arr.size() == 0 ? offset_t<void>{} : enqueueBlock<void>(arr.size() * itemRefl.template get_size<typename Backend::AddrType>(arr[0]), itemRefl.template get_alignment<typename Backend::AddrType>(), [this, arr]() {
				for (const auto& item : arr)
					process_type(item);
				}));
			backend.write(size_val_t{ arr.size() });
			backend.write(size_val_t{ arr.capacity() });
			backend.write(size_val_t{ 0ull });
		}

		template<typename T>
		inline void process_tarray(const T& arr) {
			auto itemRefl = arr.refl.get_item_type();

			backend.write(arr.size() == 0 ? offset_t<void>{} : enqueueBlock<void>(arr.size() * itemRefl.template get_size<typename Backend::AddrType>(arr[0]), itemRefl.template get_alignment<typename Backend::AddrType>(), [this, arr]() {
				for (const auto& item : arr)
					process_type(item);
				}));
			backend.write(size_val_t{ arr.size() });
			backend.write(size_val_t{ arr.capacity() });
		}

		template<ucsl::reflection::accessors::CArrayAccessor T>
		inline void process_carray(const T& arr) {
			for (const auto& item : arr)
				process_type(item);
		}

		template<ucsl::reflection::accessors::PointerAccessor T>
		inline void process_pointer(const T& obj) {
			if (obj == nullptr) {
				backend.write(offset_t<void>{});
				return;
			}

			auto target = *obj;
			auto& knownPtr = lookupPtr(target);

			if (knownPtr.resolvedTarget.has_value()) {
				backend.write(knownPtr.resolvedTarget.value());
				return;
			}

			if constexpr (obj.refl.is_weak()) {
				knownPtr.weakPtrs.push_back(backend.tellp());
				backend.write(offset_t<void>{});
			}
			else {
				auto targetRefl = obj.refl.get_target_type();
				auto targetSize = targetRefl.template get_size<typename Backend::AddrType>(target);
				auto targetAlignment = targetRefl.template get_alignment<typename Backend::AddrType>();

				if (targetSize == 0) {
					backend.write(offset_t<void>{});
					return;
				}

				auto targetOffset = enqueueBlock<void>(targetSize, targetAlignment, [this, target]() {
					process_type(target);
				});

				resolvePtr(knownPtr, targetOffset);

				target.visit([&](auto v) {
					if constexpr (decltype(v.refl)::kind == providers::TypeKind::CARRAY) {
						size_t cur = targetOffset.value();

						for (const auto& item : v) {
							resolvePtr(lookupPtr(item), offset_t<void>{ cur });
							cur += item.refl.template get_size<typename Backend::AddrType>(item);
						}
					}
				});

				backend.write(targetOffset);
			}
		}

		template<typename T>
		inline void process_union(const T& obj) {
			obj.visit([&](auto field) { process_type(field); });
		}

		template<ucsl::reflection::accessors::ValueAccessor T>
		inline void process_type(const T& obj) {
			backend.write_padding(obj.refl.template get_alignment<typename Backend::AddrType>());

			size_t typeStart = backend.tellp();
			//std::cout << std::hex << "type start at " << static_cast<size_t>(&obj) << " in, out at " << typeStart << std::endl;

			obj.visit([&](auto v) {
				if constexpr (decltype(v.refl)::kind == providers::TypeKind::PRIMITIVE) process_primitive(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::ENUM) process_enum(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::BITFIELD) process_bitfield(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::ARRAY) process_array(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::TARRAY) process_tarray(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::CARRAY) process_carray(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::POINTER) process_pointer(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::UNION) process_union(v);
				else if constexpr (decltype(v.refl)::kind == providers::TypeKind::STRUCTURE) process_struct(v);
				else static_assert(false, "invalid type kind");
			});

			backend.write_padding_bytes(obj.refl.template get_size<typename Backend::AddrType>(obj) - (backend.tellp() - typeStart));
		}

		template<ucsl::reflection::accessors::StructureAccessor T>
		inline void process_fields(const T& obj) {
			auto base = obj.get_base();

			if (base.has_value()) {
				auto baseStart = backend.tellp();
				process_fields(base.value());
				backend.write_padding_bytes(base.value().refl.template get_size<typename Backend::AddrType>(obj) - (backend.tellp() - baseStart));
			}

			obj.visit_fields([&](const auto& field, const auto& fieldRefl) {
				//std::cout << "field " << fieldRefl.get_name() << std::endl;
				process_type(field);
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
			nextOffset = backend.tellp();
			enqueueBlock<void>(obj.refl.template get_size<typename Backend::AddrType>(obj), obj.refl.template get_alignment<typename Backend::AddrType>(), [this, obj]() { process_type(obj); });
			processQueuedBlocks();
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
		void* buf = new (AllocatorSystem::get_allocator()) uint8_t[measureBinary<AddrType, endianness, byteswap_offsets, relative_offsets>(acc)]{};

		serializeBinaryToBuffer<AddrType, endianness, byteswap_offsets, relative_offsets>(buf, acc);

		return buf;
	}

	template<typename AddrType, std::endian endianness = std::endian::native, bool byteswap_offsets = true, bool relative_offsets = false>
	void* serializeBinaryToNativeBuffer(const auto& acc) {
		void* buf = new uint8_t[measureBinary<AddrType, endianness, byteswap_offsets, relative_offsets>(acc)]{};

		serializeBinaryToBuffer<AddrType, endianness, byteswap_offsets, relative_offsets>(buf, acc);

		return buf;
	}
}