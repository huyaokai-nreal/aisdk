#ifndef _NRNN_MODEL_H_
#define _NRNN_MODEL_H_

#include <memory>
#include <string>
#include <vector>

#include "nrhal_define.h"

namespace aisdk::xengine {

struct ModelConfig {
    //默认构造函数
    ModelConfig() = default;

    //复制构造函数
    ModelConfig(const ModelConfig& other)
        : model_path(other.model_path),
          model_size(other.model_size),
          vendor_type(other.vendor_type),
          dont_batch(other.dont_batch) {
        if (other.model_mem && other.model_size > 0) {
            model_mem = static_cast<char*>(malloc(other.model_size));
            if (model_mem) {
                memcpy(const_cast<char*>(model_mem), other.model_mem, other.model_size);
            }
        }
    }

    //复制赋值运算符
    ModelConfig& operator=(const ModelConfig& other) {
        if (this != &other) {
            // 释放当前对象的旧资源
            if (model_mem) {
                free(const_cast<char*>(model_mem));
            }

            // 复制新的数据
            model_path = other.model_path;
            model_size = other.model_size;
            vendor_type = other.vendor_type;
            dont_batch = other.dont_batch;

            if (other.model_mem && other.model_size > 0) {
                model_mem = static_cast<char*>(malloc(other.model_size));
                if (model_mem) {
                    memcpy(const_cast<char*>(model_mem), other.model_mem, other.model_size);
                }
            } else {
                model_mem = nullptr;
            }
        }
        return *this;
    }

    // ModelConfig 的移动构造函数
    ModelConfig(ModelConfig&& other) noexcept
        : model_path(std::move(other.model_path)),
        model_mem(other.model_mem),
        model_size(other.model_size),
        vendor_type(other.vendor_type),
        dont_batch(other.dont_batch) {

        other.model_mem = nullptr; // 置空原指针
        other.model_size = 0;
    }

    // ModelConfig 的移动赋值运算符
    ModelConfig& operator=(ModelConfig&& other) noexcept {
        if (this != &other) {
            // 释放当前对象的旧资源
            if (model_mem) free(const_cast<char*>(model_mem));

            // 接管新资源
            model_path = std::move(other.model_path);
            model_mem = other.model_mem;
            model_size = other.model_size;
            vendor_type = other.vendor_type;
            dont_batch = other.dont_batch;

            // 置空原对象
            other.model_mem = nullptr;
            other.model_size = 0;
        }

        return *this;
    }

    ~ModelConfig() {
        if (model_mem) {
            free(const_cast<char*>(model_mem));
            model_mem = nullptr;
        }
    }

    std::string model_path;
    const char *model_mem = nullptr;
    uint32_t model_size = 0;
    VendorType vendor_type = VendorType::UNKNOWN;
    bool dont_batch = false;
};

struct ModelInfo {
    uint64_t handle = 0;
};

class AIModel {
   protected:
    AIModel() = default;

   public:
    virtual ~AIModel() = default;
    virtual bool IsShared() = 0;

   public:
    bool m_success = false;
    ModelInfo m_info;
};
}  // namespace aisdk::xengine

std::shared_ptr<aisdk::xengine::AIModel> CreateModelPtr(std::string &key, aisdk::xengine::ModelConfig &config);
aisdk::xengine::Status DestoryModelPtr(std::string &key);
#endif