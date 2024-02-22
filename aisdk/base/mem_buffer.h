#ifndef _XR_MEM_BUFFER_H_
#define _XR_MEM_BUFFER_H_

#include <map>
#include <memory>
#include <mutex>

#include "log.h"

namespace aisdk::base {

struct XrMem {
    void* addr = nullptr;
    uint64_t size = 0;
};

class FixedMembuffer {
   public:
    FixedMembuffer(std::string name, uint64_t warningsize) {
        module_name = name;
        warning_size = warningsize;
    }

    ~FixedMembuffer() {
        std::lock_guard<std::mutex> guard(cache_lock);
        for (auto [k, v] : free_cache) {
            delete[] v->addr;
            delete v;
        }
    }

    std::shared_ptr<XrMem> RequestMemBlob(uint64_t alloc_size) {
        std::string mem_name;
        XrMem* find_mem = nullptr;
        {
            std::lock_guard<std::mutex> guard(cache_lock);
            for (auto [k, v] : free_cache) {
                // 这里应该拿第1个就行
                if (v->size == alloc_size) {
                    mem_name = k;
                    find_mem = v;
                    free_cache.erase(k);
                    break;
                } else {
                    AISDK_LOG_WARN("RequestMemBlob warning, name=%s, reason=(%lu != %lu)", module_name.c_str(), v->size,
                                   alloc_size);
                }
            }
        }

        if (nullptr == find_mem) {
            if ((cache_size + alloc_size) > warning_size) {
                AISDK_LOG_WARN("RequestMemBlob failure, name=%s, reason=(%lu + %lu > %lu)", module_name.c_str(),
                               cache_size, alloc_size, warning_size);
                return nullptr;
            }
            find_mem = new XrMem();
            find_mem->size = alloc_size;
            find_mem->addr = (void*)(new char[alloc_size]);
            std::lock_guard<std::mutex> guard(cache_lock);
            mem_name = module_name + std::to_string(key_id++);
            cache_size += alloc_size;
        }

        std::shared_ptr<XrMem> ret(find_mem, [=](XrMem* mem) {
            std::lock_guard<std::mutex> guard(this->cache_lock);
            this->free_cache[mem_name] = mem;
        });

        return ret;
    }

   public:
    std::string module_name;
    uint64_t key_id = 0;
    uint64_t cache_size = 0;
    uint64_t warning_size = 0;

    std::mutex cache_lock;
    std::map<std::string, XrMem*> free_cache;
};

}  // namespace aisdk::base

#endif
