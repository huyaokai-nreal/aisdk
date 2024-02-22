#ifndef _NRNN_MODEL_H_
#define _NRNN_MODEL_H_

#include <memory>
#include <string>
#include <vector>

#include "nrhal_define.h"

namespace Xengine {

struct ModelConfig {
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
}  // namespace Xengine

std::shared_ptr<Xengine::AIModel> CreateModelPtr(std::string &key, Xengine::ModelConfig &config);
Xengine::Status DestoryModelPtr(std::string &key);
#endif