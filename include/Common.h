#ifndef _TULIP_COMMON_H
#define TULIP_COMMON_H

#include <cstddef>
#include <array>
#include <atomic>

namespace Tulip_MemoryPool {
    constexpr size_t ALIGNMENT = 8;
    constexpr size_t MAX_BYTES = 256 * 1024;     //256KB
    constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT;          //ALIGNMENT 等于指针 void* 的大小

    class SizeManager {
    public:
        /*
            向上对齐到 ALIGMENT 的倍数
        */
        static size_t roundUp(size_t bytes) {
            return (bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
        }

        static size_t getIndex(size_t bytes) {
            bytes = std::max(bytes, ALIGNMENT);
            return (bytes + ALIGNMENT - 1) / ALIGNMENT - 1;
        }
    };
}

#endif