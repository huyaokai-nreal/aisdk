#pragma once

#include <memory>
#include <tuple>
#include <vector>

#include "aisdk/xengine/nr_model_mgr.h"
#include "nrcore_define.h"
#include "aisdk/base/profiling.h"
#include "aisdk/xengine/nrhal_capi_symbol.h"

namespace aisdk::algorithm {
class PipeGraphImpl {
   public:
    PipeGraphImpl() {}
    virtual ~PipeGraphImpl() {}
    virtual aisdk::algorithm::Status Init(aisdk::xengine::DlSymFuncs &funcs, aisdk::xengine::PipelineConfig &config, CameraParams &camera);
    virtual aisdk::algorithm::Status Start();
    virtual aisdk::algorithm::Status Stop();
};

class Pipeline {
   public:
    Pipeline();
    ~Pipeline();

    template <typename T>
    aisdk::algorithm::Status Init(aisdk::xengine::DlSymFuncs &funcs, aisdk::xengine::PipelineConfig &config, CameraParams &camera) {
        static_assert(std::is_base_of<PipeGraphImpl, T>::value, "T is not derived from PipeGraphImpl!");

        if (config.framework_type == aisdk::xengine::FrameworkType::MEDIAPIPE_GRAPH) {
            // 区分不同实现的pipeline
            m_graph_impl = std::make_shared<T>();
            if (m_graph_impl) {
                auto err = m_graph_impl->Init(funcs, config, camera);
                if (aisdk::algorithm::Status::SUCCESS != err) {
                    m_graph_impl = nullptr;
                    return aisdk::algorithm::Status::PIPELINE_INIT_FAILURE;
                }
            }
            return aisdk::algorithm::Status::SUCCESS;
        }

        return aisdk::algorithm::Status::PIPELINE_INIT_FAILURE;
    }

    std::shared_ptr<PipeGraphImpl> Impl() { return m_graph_impl; }

   private:
    std::shared_ptr<PipeGraphImpl> m_graph_impl = nullptr;
};

}  // namespace aisdk::algorithm
