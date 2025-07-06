#pragma once

#ifndef _TULIP_CENTRAL_CACHE_H
#define _TULIP_CENTRAL_CACHE_H

#include "Common.h"
#include <mutex>

namespace Tulip_MemoryPool {
    class CentralCache {
    public:
        static CentralCache& getInstance() {
            static CentralCache instace;
            return instace;
        }

        void* fetchRange(size_t index, size_t batchNum);
        void returnRange(void* start, size_t size, size_t index);

    private:
        CentralCache() {
            for(auto& ptr: _centralFreeList) {
                ptr.store(nullptr, std::memory_order_relaxed);
            }

            for(auto& lock: _locks) {
                lock.clear();
            }
        }

        void* fetchFromPageCache(size_t size);
    private:
        std::array<std::atomic<void*>, FREE_LIST_SIZE> _centralFreeList;

        std::array<std::atomic_flag, FREE_LIST_SIZE> _locks;
    };
}
 
#endif