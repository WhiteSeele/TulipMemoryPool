#include "../include/ThreadCache.h"
#include "../include/CentralCache.h"

namespace Tulip_MemoryPool {
    void* ThreadCache::allocate(size_t size) {
        if(size == 0) {
            size = ALIGNMENT;  //对齐内存，至少分配一个对其大小
        }

        if(size > MAX_BYTES) {
            return malloc(size);      //大对象直接从系统分配
        }

        size_t index = SizeManager::getIndex(size);

        _freeListSize[index] -- ;

        //ptr 不为空表示当前链表中有可用的内存块
        if(void* ptr = _freeList[index]) {
            _freeList[index] = *reinterpret_cast<void**>(ptr);
            return ptr;
        }

        //本地没有可用内存，从中心缓存取内存块
        return fetchFromCentralCache(index);
    }

    void ThreadCache::deallocate(void* ptr, size_t size) {
        if(size > MAX_BYTES) {
            free(ptr);
            return;
        }

        size_t index = SizeManager::getIndex(size);

        *reinterpret_cast<void**>(ptr) = _freeList[index];
        _freeList[index] = ptr;

        _freeListSize[index] ++ ;

        if(shouldReturnToCentralCache(index)) {
            returnToCentralCache(_freeList[index], size);
        }
    }

    constexpr size_t thresold = 64;  //设置返还给中心缓存内存的自由链表大小阈值
    bool ThreadCache::shouldReturnToCentralCache(size_t index) {
        return _freeListSize[index] > thresold;
    }

    void ThreadCache::returnToCentralCache(void *start, size_t size) {
        size_t index = SizeManager::getIndex(size);

        size_t alignedSize = SizeManager::roundUp(size);

        size_t batchNum = _freeListSize[index];
        if(batchNum <= 1) return;

        size_t keepNum = std::max(batchNum >> 2, size_t(1));
        size_t returnNum = batchNum - keepNum;

        char* current = static_cast<char*>(start);
        char* spliteNode = current;

        for(int i = 0; i < keepNum - 1; i ++ ) {
            spliteNode = reinterpret_cast<char*>(*reinterpret_cast<void**>(spliteNode));
            if(spliteNode == nullptr) {
                returnNum = batchNum - (i + 1);
                break;
            }
        }

        if(spliteNode != nullptr) {
            void* next = *reinterpret_cast<void**>(spliteNode);
            *reinterpret_cast<void**>(spliteNode) = nullptr;

            _freeList[index] = start;
            _freeListSize[index] = keepNum;

            if(returnNum > 0 && next != nullptr) {
                CentralCache::getInstance().returnRange(next, returnNum * alignedSize, index);
            }
        }
    }

    void* ThreadCache::fetchFromCentralCache(size_t index) {
        size_t size = (index + 1) * ALIGNMENT;
        size_t batchNum = getBatchNum(size);
        
        void* start = CentralCache::getInstance().fetchRange(index, batchNum);
        if(!start) return nullptr;

        _freeListSize[index] += batchNum;

        void* result = start;
        if(batchNum > 1) {
            _freeList[index] = *reinterpret_cast<void**>(start);
        }
        return result;
    }

    size_t ThreadCache::getBatchNum(size_t size) {
        //baseline: 每次批量获取不超过 4KB 内存
        constexpr size_t MAX_BATCH_SIZE = 4 * 1024;

        //根据对象大小设置合理的基准批量参数
        size_t baseNum;
        //2KB
        if(size <= 32) baseNum = 64;
        else if(size <= 64) baseNum = 32;
        else if(size <= 128) baseNum = 16;
        else if(size <= 256) baseNum = 8;
        else if(size <= 512) baseNum = 4;
        else if(size <= 1024) baseNum = 2;
        else baseNum = 1;

        size_t maxNum = std::max(size_t(1), MAX_BATCH_SIZE / size);
        return std::max(size_t(1), std::min(maxNum, baseNum));
    }
}