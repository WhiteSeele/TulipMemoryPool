#include "../include/PageCache.h"
#include <sys/mman.h>
#include <cstring>

namespace Tulip_MemoryPool {
    void* PageCache::allocateSpan(size_t numPages) {
        std::lock_guard<std::mutex> lock(_mutex);

        auto iter = _freeSpans.lower_bound(numPages);
        if(iter != _freeSpans.end()) {
            Span* span = iter -> second;
            //取出的 Span 从原有的空闲链表中移除
            if(span -> next) {
                _freeSpans[iter -> first] = span -> next;
            } else {
                _freeSpans.erase(iter);
            }
            //span 大于需要的 numPages 进行分割
            if(span -> numPages > numPages) {
                Span* newSpan = new Span;
                newSpan -> pageAddr = static_cast<char*>(span -> pageAddr) + numPages * PAGE_SIZE;
                newSpan -> numPages = span -> numPages - numPages;
                newSpan -> next = nullptr;

                //超出部分放回链表头部
                auto& list = _freeSpans[newSpan->numPages];
                newSpan -> next = list;
                list = newSpan;

                span -> numPages = numPages;
            }
            //记录Span信息用于回收
            _spanMap[span -> pageAddr] = span;
            return span -> pageAddr;
        }

        void* sysMemory = systemAlloc(numPages);
        if(!sysMemory) return nullptr;

        Span* span = new Span;
        span -> pageAddr = sysMemory;
        span -> numPages = numPages;
        span -> next = nullptr;

        _spanMap[sysMemory] = span;
        return sysMemory; 
    }

    void PageCache::deallocateSpan(void* ptr, size_t numPages) {
        std::lock_guard<std::mutex> lock(_mutex);

        auto iter = _spanMap.lower_bound(ptr);
        if(iter == _spanMap.end()) return;

        Span* span = iter -> second;

        void* nextAddr = static_cast<char*>(ptr) + numPages * PAGE_SIZE;
        auto nextIt = _spanMap.find(nextAddr);

        if(nextIt != _spanMap.end()) {
            Span* nextSpan = nextIt -> second;

            //检查是否是头结点
            bool isFree = false;
            auto& nextList = _freeSpans[nextSpan -> numPages];

            if(nextList == nextSpan) {
                nextList = nextSpan -> next;
                isFree = true;
            } else if(nextList) {   //链表非空时遍历
                Span* prev = nextList;
                while(prev -> next) {
                    if(prev -> next == nextSpan) {
                        prev -> next = nextSpan -> next;
                        isFree = true;
                        break;
                    }
                    prev = prev -> next;
                }
            }

            if(isFree) {
                span -> numPages += nextSpan -> numPages;
                _spanMap.erase(nextAddr);
                delete nextSpan;
            }
        }
        //头插法插入空闲链表
        auto& list = _freeSpans[span->numPages];
        span->next = list;
        list = span;

    }

    void* PageCache::systemAlloc(size_t numPages) {
        size_t size = numPages * PAGE_SIZE;

        void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if(ptr == MAP_FAILED) return nullptr;

        memset(ptr, 0, size);
        return ptr;
    }
}