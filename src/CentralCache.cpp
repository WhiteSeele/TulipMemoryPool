#include "../include/CentralCache.h"
#include "../include/PageCache.h"
#include <cassert>
#include <thread>

namespace Tulip_MemoryPool {
    //从 PageCache 获取 span 的大小(以页为单位)
    static const size_t SPAN_PAGES = 8;

    void* CentralCache::fecthRange(size_t index, size_t batchNum) {
        if(index >= FREE_LIST_SIZE || batchNum == 0)
            return nullptr;

        while(_locks[index].test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();          //添加线程让步，避免忙等
        }

        void* result = nullptr;
        try {
            result = _centralFreeList[index].load(std::memory_order_relaxed);
            if(!result) {
                size_t size = (index + 1) * ALIGNMENT;
                result = fetchFromPageCache(size);
                
                if(!result) {
                    _locks[index].clear(std::memory_order_release);
                    return nullptr;
                }

                char* start = static_cast<char*>(result);
                size_t totalBlocksNum = (SPAN_PAGES * PageCache::PAGE_SIZE) / size;
                size_t allocBlocksNum = std::min(batchNum, totalBlocksNum);

                //构建返回给 ThreadCache 的内存块结果
                if(allocBlocksNum > 1) {
                    //确保至少有两个块才构建链表
                    //构建链表
                    for(size_t i = 1; i < allocBlocksNum; i ++ ) {
                        void* current = start + (i - 1) * size;
                        void* next = start + i * size;
                        *reinterpret_cast<void**>(current) = next;
                    }
                    *reinterpret_cast<void**>(start + (allocBlocksNum - 1) * size) = nullptr;
                }

                //构建留在 CentralCache 的链表
                if(totalBlocksNum > allocBlocksNum) {
                    void* remainStart = start + allocBlocksNum * size;
                    for(size_t i = allocBlocksNum + 1; i < totalBlocksNum; i ++ ) {
                        void* current = start + (i - 1) * size;
                        void* next = start + i * size;
                        *reinterpret_cast<void**>(current) = next;
                    }
                    *reinterpret_cast<void**>(start + (totalBlocksNum - 1) * size) = nullptr;

                    _centralFreeList[index].store(remainStart, std::memory_order_release);
                }
            } else {  //中心缓存存在index对应大小的内存块
                void* current = result;
                void* prev = nullptr;
                size_t count = 0;

                while(current && count < batchNum) {
                    prev = current;
                    current = *reinterpret_cast<void**>(current);
                    count ++ ;
                }
                //中心缓存链表的内存块数大于batchNum，断开链表
                if(prev) {
                    *reinterpret_cast<void**>(prev) = nullptr;
                }
                _centralFreeList[index].store(current, std::memory_order_release);
            }
        } catch(...) {
            _locks[index].clear(std::memory_order_release);
            throw;
        }

        _locks[index].clear(std::memory_order_release);
        return result;
    }

    void CentralCache::returnRange(void* start, size_t size, size_t index) {
        if(!start || index >= FREE_LIST_SIZE)
            return;
        
        while(_locks[index].test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        try {
            void* end = start;
            size_t count = 1;

            while(*reinterpret_cast<void**>(end) != nullptr && count < size) {
                end = *reinterpret_cast<void**>(end);
                count ++ ;
            }

            void* current = _centralFreeList[index].load(std::memory_order_relaxed);
            *reinterpret_cast<void**>(end) = current;   //原来的链表接入尾部
            _centralFreeList[index].store(start, std::memory_order_release); //归还的链表头设为新的链表头
        } catch(...) {
            _locks[index].clear(std::memory_order_release);
            throw;
        }
        _locks[index].clear(std::memory_order_release);

    }

    void* CentralCache::fetchFromPageCache(size_t size) {
        size_t numPages = (size + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;

        if(size <= SPAN_PAGES * PageCache::PAGE_SIZE) {
            //小于等于32KB的请求，使用固定8页
            return PageCache::getInstance().allocateSpan(SPAN_PAGES);
        } else {
            return PageCache::getInstance().allocateSpan(numPages);
        }
    }
}