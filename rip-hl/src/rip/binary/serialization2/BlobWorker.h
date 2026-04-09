#pragma once

#include <map>
#include <queue>
#include <functional>
#include <cassert>
#include <rip/util/memory.h>

namespace rip::binary {
	struct BlockAllocationData {
		size_t size{};
		size_t alignment{};
	};

	struct SequentialBlockAllocator {
		size_t nextOffset{};

		size_t allocate(BlockAllocationData allocationData) {
			size_t offset = align(nextOffset, allocationData.alignment);
			nextOffset = offset + allocationData.size;
			return offset;
		}
	};

	template<typename T = void>
	struct SequentialMemoryBlockAllocator {
		T* origin{};
		SequentialBlockAllocator seqAllocator{};

		T* allocate(BlockAllocationData allocationData) {
			return addptr(origin, seqAllocator.allocate(allocationData));
		}
	};

	template<typename GameInterface, typename T = void>
	struct HeapBlockAllocator {
		size_t sizeRequired{};
		std::vector<T*> live_objs{};

		~HeapBlockAllocator() {
			cleanup();
		}

		T* allocate(BlockAllocationData allocationData) {
			size_t offset = align(sizeRequired, allocationData.alignment);
			sizeRequired = offset + allocationData.size;

			T* res = (T*)GameInterface::AllocatorSystem::get_allocator()->Alloc(allocationData.size, allocationData.alignment);
			live_objs.push_back(res);
			return res;
		}

		void cleanup() {
			for (auto* ptr : live_objs)
				GameInterface::AllocatorSystem::get_allocator()->Free(ptr);

			live_objs.clear();
		}
	};

	enum class SchedulingType {
		IMMEDIATE,
		DEFERRED,
		DEFERRED_ALLOCATION,
	};

	template<typename T, typename Allocator>
	class ImmediateBlobWorkerScheduler {
		Allocator& allocator;
	public:
		ImmediateBlobWorkerScheduler(Allocator& allocator) : allocator{ allocator } {}

		void enqueueBlock(auto guard, auto storeOffset, auto allocationDataGetter, auto processFunc) {
			if (!guard())
				return;

			auto allocationData = allocationDataGetter();
			T offset = allocator.allocate(allocationData);
			storeOffset(offset);
			processFunc(offset, allocationData.alignment);
		}

		void processQueuedBlocks() {
		}
	};

	template<typename T, typename Allocator>
	class DeferredBlobWorkerScheduler {
		struct WorkQueueEntry {
			T offset;
			size_t alignment;
			std::function<void(T, size_t)> processFunc;
		};

		std::queue<WorkQueueEntry> workQueue{};
		Allocator& allocator;

	public:
		DeferredBlobWorkerScheduler(Allocator& allocator) : allocator{ allocator } {}

		void enqueueBlock(auto guard, auto storeOffset, auto allocationDataGetter, auto processFunc) {
			if (!guard())
				return;

			auto allocationData = allocationDataGetter();
			T offset = allocator.allocate(allocationData);
			storeOffset(offset);
			workQueue.push(WorkQueueEntry{ offset, allocationData.alignment, processFunc });
		}

		void processQueuedBlocks() {
			while (!workQueue.empty()) {
				auto chunk = workQueue.front();
				workQueue.pop();

				chunk.processFunc(chunk.offset, chunk.alignment);
			}
		}
	};

	template<typename T, typename Allocator>
	class DeferredAllocationBlobWorkerScheduler {
		struct WorkQueueEntry {
			std::function<bool()> guard;
			std::function<void(T)> storeOffset;
			std::function<BlockAllocationData()> allocationDataGetter;
			std::function<void(T, size_t)> processFunc;
		};

		std::queue<WorkQueueEntry> workQueue{};
		Allocator& allocator;

	public:
		DeferredAllocationBlobWorkerScheduler(Allocator& allocator) : allocator{ allocator } {}

		void enqueueBlock(auto guard, auto storeOffset, auto allocationDataGetter, auto processFunc) {
			workQueue.push(WorkQueueEntry{ guard, storeOffset, allocationDataGetter, processFunc });
		}

		void processQueuedBlocks() {
			while (!workQueue.empty()) {
				auto chunk = workQueue.front();
				workQueue.pop();

				if (!chunk.guard())
					continue;
				
				chunk.storeOffset((T)0xFAFAFAFA);
				BlockAllocationData allocationData = chunk.allocationDataGetter();
				T offset = allocator.allocate(allocationData);
				chunk.storeOffset(offset);
				chunk.processFunc(offset, allocationData.alignment);
			}
		}
	};

	template<typename T = size_t, typename Allocator = SequentialBlockAllocator, template<typename, typename> typename Scheduler = DeferredBlobWorkerScheduler>
	class BlobWorker {
	public:
		Allocator allocator;

	private:
		Scheduler<T, Allocator> scheduler;

	public:
		template<typename... Args>
		BlobWorker(Args&&... args) : allocator{ std::forward<Args>(args)... }, scheduler{ allocator } {}

		T enqueueBlock(size_t size, size_t alignment, auto processFunc) {
			T offset{};
			enqueueBlock(offset, size, alignment, processFunc);
			return offset;
		}

		void enqueueBlock(T& offset, size_t size, size_t alignment, auto processFunc) {
			enqueueBlock(offset, [size, alignment]() { return BlockAllocationData{ size, alignment }; }, processFunc);
		}

		void enqueueBlock(T& offset, auto allocationDataGetter, auto processFunc) {
			enqueueBlock([&offset](T off) { offset = off; }, allocationDataGetter, processFunc);
		}

		void enqueueBlock(auto storeOffset, auto allocationDataGetter, auto processFunc) {
			enqueueBlock([]() { return true; }, storeOffset, allocationDataGetter, processFunc);
		}

		void enqueueBlock(auto guard, auto storeOffset, auto allocationDataGetter, auto processFunc) {
			scheduler.enqueueBlock(guard, storeOffset, allocationDataGetter, processFunc);
		}

		void processQueuedBlocks() {
			scheduler.processQueuedBlocks();
		}
	};
}
