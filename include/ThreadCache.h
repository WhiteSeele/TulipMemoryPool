#ifndef _THREADCACHE_H
#define _THREADCACHE_H

#include "Common.h"
#include <array>


namespace Tulip_MemoryPool {
    class ThreadCache {
    public:
        //单例模式，每个线程一个实例
        static ThreadCache* getInstance() {
            static thread_local ThreadCache instance;
            return &instance;
        }

        void* allocate(size_t size);
        void deallocate(void* ptr, size_t size);
    private:
        ThreadCache() = default;
        //从中心缓存取内存
        void* fetchFromCentralCache(size_t index);
        //归还内存到中心内存
        void returnToCentralCache(void* start, size_t size);
        //计算批量获取内存块的数量
        size_t getBatchNum(size_t size);
        //判断是否需要归还内存给中心内存
        bool shouldReturnToCentralCache(size_t index);

        std::array<void*, FREE_LIST_SIZE> _freeList;
        std::array<size_t, FREE_LIST_SIZE> _freeListSize;  //自由链表大小统计
    };
}

#endif
