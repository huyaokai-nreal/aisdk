#include "nrnn_model.h"

#include <atomic>

#if defined(HAVE_HAL_MNN)
#include "aisdk/xengine/nn/vendor_mnn/mnn_model.h"
#endif

#if defined(HAVE_HAL_RKNN)
#include "aisdk/xengine/nn/vendor_rknn/rknn_model.h"
#endif

#if defined(HAVE_HAL_SNPE)
#if SNPE_VERSION < 2000
#include "aisdk/xengine/nn/vendor_snpe/snpe_model.h"
#else
#include "aisdk/xengine/nn/vendor_snpe2/snpe_model.h"
#endif
#endif

static std::atomic<uint32_t> g_newkey_id(0);
static std::map<std::string, std::weak_ptr<aisdk::xengine::AIModel>> g_hasmod;

std::shared_ptr<aisdk::xengine::AIModel> CreateModelPtr(std::string &key, aisdk::xengine::ModelConfig &config) {
    bool is_newkey = false;
    std::shared_ptr<aisdk::xengine::AIModel> ret = nullptr;
    if (g_hasmod.find(key) != g_hasmod.end()) {
        if (!g_hasmod[key].expired()) {
            auto tmp = g_hasmod[key].lock();
            if (true == tmp->IsShared()) {
                return tmp;
            } else {
                // 存在但不能共享，需要去重建并且重新生成key
                is_newkey = true;
            }
        } else {
            g_hasmod.erase(key);
        }
    }

    if (config.vendor_type == aisdk::xengine::VendorType::MNN) {
#if defined(HAVE_HAL_MNN)
        ret = std::make_shared<aisdk::xengine::MNN_AIModel>(config);
#endif
    } else if (config.vendor_type == aisdk::xengine::VendorType::ROCKCHIP) {
#if defined(HAVE_HAL_RKNN)
        ret = std::make_shared<aisdk::xengine::RKNN_AIModel>(config);
#endif
    } else if (config.vendor_type == aisdk::xengine::VendorType::SNPE) {
#if defined(HAVE_HAL_SNPE)
        ret = std::make_shared<aisdk::xengine::SNPE_AIModel>(config);
#endif
    }

    if (ret) {
        if (is_newkey) {
            key = key + "_newkey_" + std::to_string(g_newkey_id.fetch_add(1));
        }
        g_hasmod[key] = ret;
    }
    return ret;
}

aisdk::xengine::Status DestoryModelPtr(std::string &key) {
    if (g_hasmod.find(key) != g_hasmod.end()) {
        if (g_hasmod[key].use_count() == 0) {
            g_hasmod.erase(key);
        }
        return aisdk::xengine::Status::SUCCESS;
    }
    return aisdk::xengine::Status::FAILURE;
}