#pragma once
#include <map>
#include <memory>
#include <mutex>

#include "log.h"

namespace aisdk::base {

// Structure to hold memory block information
struct XrMem {
    void* addr = nullptr; // Pointer to the memory address
    uint64_t size = 0;    // Size of the memory block
};

// Class to manage fixed-size memory buffers
class FixedMembuffer {
   public:
    // Constructor for FixedMembuffer
    FixedMembuffer(std::string name, uint64_t warningsize) {
        module_name = name;
        warning_size = warningsize;
    }

    // Destructor for FixedMembuffer
    ~FixedMembuffer() {
        std::lock_guard<std::mutex> guard(cache_lock);
        for (auto [k, v] : free_cache) {
            delete[] (char*)v->addr; // Free the allocated memory
            delete v;                // Free the XrMem structure
        }
    }

    // Method to request a memory block of a specific size
    std::shared_ptr<XrMem> RequestMemBlob(uint64_t alloc_size) {
        std::string mem_name;
        XrMem* find_mem = nullptr;
        {
            std::lock_guard<std::mutex> guard(cache_lock);
            for (auto [k, v] : free_cache) {
                // Attempt to reuse the first available memory block of the requested size
                if (v->size == alloc_size) {
                    mem_name = k;
                    find_mem = v;
                    free_cache.erase(k); // Remove the memory block from the free cache
                    break;
                } else {
                    AISDK_LOG_WARN("RequestMemBlob warning, name={}, reason=({} != {})", module_name.c_str(), v->size,
                                   alloc_size);
                }
            }
        }

        if (nullptr == find_mem) {
            if ((cache_size + alloc_size) > warning_size) {
                AISDK_LOG_WARN("RequestMemBlob failure, name={}, reason=({} + {} > {})", module_name.c_str(),
                               cache_size, alloc_size, warning_size);
                return nullptr;
            }
            find_mem = new XrMem(); // Allocate a new memory block
            find_mem->size = alloc_size;
            find_mem->addr = (void*)(new char[alloc_size]); // Allocate memory for the block
            std::lock_guard<std::mutex> guard(cache_lock);
            mem_name = module_name + std::to_string(key_id++); // Generate a unique name for the memory block
            cache_size += alloc_size; // Update the total cache size
        }

        std::shared_ptr<XrMem> ret(find_mem, [=](XrMem* mem) {
            std::lock_guard<std::mutex> guard(this->cache_lock);
            this->free_cache[mem_name] = mem; // Return the memory block to the free cache when done
        });

        return ret; // Return the memory block as a shared pointer
    }

   public:
    std::string module_name; // Name of the module using the memory buffer
    uint64_t key_id = 0;      // Unique identifier for each memory block
    uint64_t cache_size = 0; // Total size of allocated memory blocks
    uint64_t warning_size = 0; // Threshold size beyond which warnings are logged

    std::mutex cache_lock; // Mutex to protect concurrent access to the cache
    std::map<std::string, XrMem*> free_cache; // Cache of free memory blocks
};

}  // namespace aisdk::base

