#pragma once

#include <memory>

#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/xengine/nr_model_mgr.h"
#include "aisdk/xengine/nrhal_capi_symbol.h"
#include "aisdk/base/log.h"

namespace aisdk::task {
using CameraParams = algorithm::CameraParams;
class PipeGraphImpl {
   public:
    PipeGraphImpl() {}
    virtual ~PipeGraphImpl() {}
    virtual aisdk::algorithm::Status Init(aisdk::xengine::DlSymFuncs &funcs, aisdk::xengine::PipelineConfig &config,
                                          CameraParams &camera);
    virtual aisdk::algorithm::Status Start();
    virtual aisdk::algorithm::Status Stop();
};

class Pipeline {
   public:
    Pipeline();
    ~Pipeline();

    template <typename T>
    aisdk::algorithm::Status Init(aisdk::xengine::DlSymFuncs &funcs, aisdk::xengine::PipelineConfig &config,
                                  CameraParams &camera) {
        static_assert(std::is_base_of<PipeGraphImpl, T>::value, "T is not derived from PipeGraphImpl!");

        if (config.framework_type == aisdk::xengine::FrameworkType::XGRAPH) {
            // 区分不同实现的pipeline
            m_graph_impl = std::make_shared<T>();
            if (m_graph_impl) {
                auto err = m_graph_impl->Init(funcs, config, camera);
                if (aisdk::algorithm::Status::SUCCESS != err) {
                    m_graph_impl = nullptr;
                    AISDK_LOG_ERROR("m_graph_impl init failed, err: {}", static_cast<int>(err));
                    return aisdk::algorithm::Status::PIPELINE_INIT_FAILURE;
                }
            }
            return aisdk::algorithm::Status::SUCCESS;
        }

        AISDK_LOG_ERROR("pipeline init failed, config.frameword_type is not XGRAPH, config.framwork_type:{}", static_cast<int>(config.framework_type));
        return aisdk::algorithm::Status::PIPELINE_INIT_FAILURE;
    }

    std::shared_ptr<PipeGraphImpl> Impl() { return m_graph_impl; }

   private:
    std::shared_ptr<PipeGraphImpl> m_graph_impl = nullptr;
};

}  // namespace aisdk::task
