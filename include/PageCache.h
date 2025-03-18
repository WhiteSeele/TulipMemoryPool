#ifndef _TULIP_PAGECACHE_H
#define _TULIP_PAGECACHE_H

#include "Common.h"
#include <map>

namespace Tulip_MemoryPool {
    class PageCache {
    public:
        static const size_t PAGE_SIZE = 4096;

        static PageCache& getInstance() {
            static PageCache instance;
            return instance;
        }

        void* allocateSpan(size_t numPages);
        void deallocateSpan(void* ptr, size_t numPages);

    private:
        PageCache() = default;

        void* systemAlloc(size_t numPages);

        struct Span {
            void* pageAddr;          //页起始地址
            size_t numPages;         //页数
            Span* next;              //链表指针
        };

        std::map<size_t, Span*> _freeSpans;   //按页数管理空闲 Span ，不同页数对应不同Span链表
        std::map<void*, Span*> _spanMap;      //页号到Span的映射
        std::mutex _mutex;
    };
}

#endif