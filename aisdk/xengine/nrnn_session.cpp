#include "nrnn_session.h"

#include "aisdk/base/log.h"

#if defined(HAVE_HAL_MNN)
#include "aisdk/xengine/nn/vendor_mnn/mnn_session.h"
#endif

#if defined(HAVE_HAL_RKNN)
#include "aisdk/xengine/nn/vendor_rknn/rknn_session.h"
#endif

#if defined(HAVE_HAL_SNPE)
#if SNPE_VERSION < 2000
#include "aisdk/xengine/nn/vendor_snpe/snpe_session.h"
#else
#include "aisdk/xengine/nn/vendor_snpe2/snpe_session.h"
#endif
#endif

#include "aisdk/xengine/nn/vendor_xrnn/xrnn_session.h"

namespace Xengine {

Inference::Inference(ModelConfig &Mconfig, SessionConfig &Sconfig) {
    m_mconfig = Mconfig;
    m_sconfig = Sconfig;
    m_modelimpl = nullptr;
    m_sessionimpl = nullptr;
}

Inference::~Inference() {
    m_sessionimpl = nullptr;
    if (m_modelimpl) {
        m_modelimpl = nullptr;
        DestoryModelPtr(m_netname);
    }
}

Status Inference::Init(std::string &netname) {
    std::string new_netname = netname;
    m_modelimpl = CreateModelPtr(new_netname, m_mconfig);
    if (m_modelimpl) {
        m_netname = new_netname;
        if (m_mconfig.vendor_type == VendorType::MNN) {
#if defined(HAVE_HAL_MNN)
            m_sessionimpl = std::make_shared<Xengine::MNN_Session>();
#endif
        } else if (m_mconfig.vendor_type == VendorType::ROCKCHIP) {
#if defined(HAVE_HAL_RKNN)
            m_sessionimpl = std::make_shared<Xengine::RKNN_Session>();
#endif
        } else if (m_mconfig.vendor_type == VendorType::SNPE) {
#if defined(HAVE_HAL_SNPE)
            m_sessionimpl = std::make_shared<Xengine::SNPE_Session>();
#endif
        } else if (m_mconfig.vendor_type == VendorType::XREAL) {
            m_sessionimpl = std::make_shared<Xengine::XRNN_Session>();
        }

        if (m_sessionimpl) {
            auto ret = m_sessionimpl->Init(m_modelimpl, m_sconfig);
            return ret;
        } else {
            return Status::MODEL_INIT_FAILURE;
        }
    }

    return Status::MODEL_LOAD_FAILURE;
}

IoTensors Inference::GetInputTensors() {
    if (m_sessionimpl) {
        return m_sessionimpl->m_in;
    }
    return IoTensors();
}

IoTensors Inference::GetOutputTensors() {
    if (m_sessionimpl) {
        return m_sessionimpl->m_out;
    }

    return IoTensors();
}

uint32_t Inference::GetInputTensorIndex(const std::string &tensorname) {
    if (m_sessionimpl) {
        for (uint32_t i = 0; i < m_sessionimpl->m_in.m_multishape_num; i++) {
            if (m_sessionimpl->m_in.m_tensors[i].m_name == tensorname) {
                return i;
            }
        }
    }
    AISDK_LOG_TRACE("Inference::GetInputTensorIndex error !!!!!");
    return 0;
}

uint32_t Inference::GetOutputTensorIndex(const std::string &tensorname) {
    if (m_sessionimpl) {
        for (uint32_t i = 0; i < m_sessionimpl->m_out.m_multishape_num; i++) {
            if (m_sessionimpl->m_out.m_tensors[i].m_name == tensorname) {
                return i;
            }
        }
    }
    AISDK_LOG_TRACE("Inference::GetOutputTensorIndex error !!!!!");
    return 0;
}

Status Inference::RunNet() { return m_sessionimpl->Forword(m_modelimpl->m_info); }

}  // namespace Xengine
