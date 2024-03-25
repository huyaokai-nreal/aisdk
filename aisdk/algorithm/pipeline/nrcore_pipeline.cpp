#include "nrcore_pipeline.h"

#include <type_traits>

#include "../core/nrcore_pipeline_mediapipe_service.h"
#include "aisdk/algorithm/common/NR_GlobalCoordService.h"
#include "aisdk/base/log.h"

namespace aisdk::algorithm {

void SetGlobalCameraParams(CameraParams &camera) {
    Eigen::Matrix3f gl_R_cv;
    uint32_t generate_method = (uint32_t)camera.m_params["generate_method"][0];
    if (generate_method == 1) {
        gl_R_cv << 1, 0, 0, 0, -1, 0, 0, 0, -1;
    } else if (generate_method == 2) {
        gl_R_cv << 1, 0, 0, 0, 1, 0, 0, 0, 1;
    }

    Eigen::Vector3f gl_t_cv = {0., 0., 0.};

    Eigen::Quaternionf glL_R_glR = {camera.m_params["glL_R_glR"][0], camera.m_params["glL_R_glR"][1],
                                    camera.m_params["glL_R_glR"][2], camera.m_params["glL_R_glR"][3]};

    Eigen::Vector3f glL_t_glR = {camera.m_params["glL_t_glR"][0], camera.m_params["glL_t_glR"][1],
                                 camera.m_params["glL_t_glR"][2]};

    Eigen::Quaternionf glH_R_glL = {camera.m_params["glH_R_glL"][0], camera.m_params["glH_R_glL"][1],
                                    camera.m_params["glH_R_glL"][2], camera.m_params["glH_R_glL"][3]};

    Eigen::Vector3f glH_t_glL = {camera.m_params["glH_t_glL"][0], camera.m_params["glH_t_glL"][1],
                                 camera.m_params["glH_t_glL"][2]};

    GlobalCoordService::getInstance()->setTransform(XrealCoordSystem::CV_LEFT_CAM, XrealCoordSystem::GL_LEFT_CAM,
                                                    gl_R_cv, gl_t_cv);
    GlobalCoordService::getInstance()->setTransform(XrealCoordSystem::GL_RIGHT_CAM, XrealCoordSystem::GL_LEFT_CAM,
                                                    glL_R_glR, glL_t_glR);
    GlobalCoordService::getInstance()->setTransform(XrealCoordSystem::GL_LEFT_CAM, XrealCoordSystem::GL_HEAD, glH_R_glL,
                                                    glH_t_glL);
    GlobalCoordService::getInstance()->setTransform(XrealCoordSystem::CV_RIGHT_CAM, XrealCoordSystem::GL_RIGHT_CAM,
                                                    gl_R_cv, gl_t_cv);
    GlobalCoordService::getInstance()->finalize();
}

aisdk::algorithm::Status PipeGraphImpl::Init(aisdk::xengine::DlSymFuncs &funcs, aisdk::xengine::PipelineConfig &config,
                                             CameraParams &camera) {
    // aisdk::algorithm::XrMediaServiceUtils::SavePipelineConfig(this, funcs, config, camera);
    // (void*)0x202310
    aisdk::algorithm::XrMediaServiceUtils::SavePipelineConfig((void *)0x202310, funcs, config, camera);
    SetGlobalCameraParams(camera);
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
