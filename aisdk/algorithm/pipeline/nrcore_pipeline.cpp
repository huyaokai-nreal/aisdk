#include "nrcore_pipeline.h"

#include <type_traits>

#include "../core/nrcore_pipeline_mediapipe_service.h"
#include "aisdk/base/log.h"

#ifdef HAVE_HANDTRACKING
#include "handtracking_mediapipe_graph.h"
#endif

namespace aisdk::algorithm {

aisdk::algorithm::Status PipeGraphImpl::Init(aisdk::xengine::DlSymFuncs &funcs, aisdk::xengine::PipelineConfig &config,
                                             CameraParams &camera) {
    // aisdk::algorithm::XrMediaServiceUtils::SavePipelineConfig(this, funcs, config, camera);
    // (void*)0x202310
    aisdk::algorithm::XrMediaServiceUtils::SavePipelineConfig((void *)0x202310, funcs, config, camera);
    return aisdk::algorithm::Status::SUCCESS;
}

aisdk::algorithm::Status PipeGraphImpl::Start() { return aisdk::algorithm::Status::FAILURE; }

aisdk::algorithm::Status PipeGraphImpl::Stop() { return aisdk::algorithm::Status::FAILURE; }

Pipeline::Pipeline() { m_graph_impl = nullptr; }

Pipeline::~Pipeline() {
    if (m_graph_impl) {
        m_graph_impl->Stop();
        m_graph_impl = nullptr;
    }
}

}  // namespace aisdk::algorithm
