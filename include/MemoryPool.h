#pragma once

#ifndef _MEMORY_POOL_H
#define _MEMORY_POOL_H

#include "ThreadCache.h"

namespace Tulip_MemoryPool {
    class MemoryPool {
    public:
        static void* allocate(size_t size) {
            return ThreadCache::getInstance() -> allocate(size);
        }

        static void deallocate(void* ptr, size_t size) {
            ThreadCache::getInstance() -> deallocate(ptr, size);
        }
    };
}

#endif