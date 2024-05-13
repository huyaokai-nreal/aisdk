#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "opencv2/opencv.hpp"
#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"
#include "aisdk/xengine/nr_model_mgr.h"
#include "aisdk/xengine/nrhal_net.h"
#include "aisdk/algorithm/model/hand_detect.h"
// using namespace aisdk::base;

TEST_CASE("testing create netalgo") {
    aisdk::base::Logger::GetInstance()->SetLogAllLevel(true);  // 打开日志
    auto& profcnf = aisdk::base::DebugProfiling::Get().GetOpt();
    profcnf.loglevel_trace = true;
    profcnf.aisdk_init_report = true;
    _ZN2NR200TK7FUNC005E(profcnf);

    aisdk::xengine::PlatformStatus* platform = _ZN2NR200TK7FUNC001E();
    AISDK_LOG_INFO("is_snpe_support={}",platform->is_snpe_support);

    std::string tar_name(NAME_TO_STRING(DEFAULT_PIPELINE_TAR_NAME));
    aisdk::xengine::AnalysisTar *tar_handle = _ZN2NR200TK7FUNC007E();
    bool ret = tar_handle->TarMem(tar_name.c_str());
    AISDK_LOG_INFO("TarMem ret={}",ret);
    CHECK(ret);
    auto& pipelineConfig = tar_handle->GetPipelineConfig();
    AISDK_LOG_INFO("pipelineConfig.size={}",pipelineConfig.size());
    CHECK(pipelineConfig.size());

    std::shared_ptr<aisdk::algorithm::HandDetectNetv2> detect_handle = std::make_shared<aisdk::algorithm::HandDetectNetv2>();
    for(uint32_t j = 0; j < pipelineConfig.size(); j++) {
        if (j != 1)
            continue;

        auto& config = pipelineConfig[j];
        auto& global_shared_config = *config.global_shared_config;
        AISDK_LOG_INFO("j={} pipeline_name={}", j, config.pipeline_name.c_str());

        for (uint32_t i = 0; i < global_shared_config.netalgo_model_name.size(); i++) {
            AISDK_LOG_INFO("i={} node_name={}", i, global_shared_config.netalgo_model_name[i].c_str());
            // netalgo_node的初始化
            if ("detect" == global_shared_config.netalgo_model_name[i]) {
                auto &algo_tp = global_shared_config.netalgo_config[i];
                aisdk::xengine::ModelConfig& pa = std::get<0>(algo_tp);
                aisdk::xengine::SessionConfig& pb = std::get<1>(algo_tp);
                aisdk::xengine::NetAlgoConfig& pc = std::get<2>(algo_tp);
                AISDK_LOG_INFO("algo_name={}", pc.algo_name.c_str());
                if (pc.has_param) {
                    AISDK_LOG_INFO("algo_param={}", pc.algo_param.c_str());
                }

                aisdk::xengine::BaseNetAlgo* basealgo = _ZN2NR200TK7FUNC002E(nullptr, nullptr, nullptr, nullptr);
                CHECK(basealgo);

                auto ret = detect_handle->Init(pc, pa, pb);
                if (ret.ok()) {
                    aisdk::xengine::IoTensors iots = basealgo->GetInputTensors();
                    PrintfHalIoTensors(iots);
                    aisdk::xengine::IoTensors oots = basealgo->GetOutputTensors();
                    PrintfHalIoTensors(oots);
                }

                // 读取图像
                cv::Mat src_img1 = cv::imread("./seq_left_000000_detect.jpg", cv::IMREAD_GRAYSCALE);
                cv::Mat src_img2 = cv::imread("./seq_right_000000_detect.jpg", cv::IMREAD_GRAYSCALE);

                aisdk::algorithm::Image img1(src_img1);
                aisdk::algorithm::Image img2(src_img2);

                std::vector<aisdk::algorithm::Image> image_data;
                image_data.emplace_back(std::move(img1));
                image_data.emplace_back(std::move(img2));

                // 模型推理
                std::unique_ptr<aisdk::algorithm::DetOutputInternal> output_buffer_ = std::make_unique<aisdk::algorithm::DetOutputInternal>();
                output_buffer_->clear();
                auto &result = *output_buffer_;

                detect_handle->Inference(image_data, result);

                AISDK_LOG_TRACE("[HandDetectionCalculator], lhand size : {}, rhand size : {}", result.images_lhand_rects.size(), result.images_rhand_rects.size());
                AISDK_LOG_TRACE("[HandDetectionCalculator], lhand[0] size : {}, lhand[1] size : {}", result.images_lhand_rects[0].size(), result.images_lhand_rects[1].size());

                // 结果处理
                if (result.images_lhand_rects.size() == 2 && result.images_lhand_rects[0].size() > 0 && result.images_lhand_rects[1].size() > 0) {
                    AISDK_LOG_TRACE("[HandDetectionCalculator], OK!!!");
                    // result.lhand_valid = true;

                    auto lcam_lhand = result.images_lhand_rects[0][0];
                    AISDK_LOG_TRACE("[HandDetectionCalculator] left hand + left cam: x: {}, y: {}, w: {}, h: {}", lcam_lhand.x, lcam_lhand.y, lcam_lhand.w, lcam_lhand.h);
                    auto rcam_lhand = result.images_lhand_rects[1][0];
                    AISDK_LOG_TRACE("[HandDetectionCalculator] left hand + right cam: x: {}, y: {}, w: {}, h: {}", rcam_lhand.x, rcam_lhand.y, rcam_lhand.w, rcam_lhand.h);
                } else {
                    AISDK_LOG_TRACE("[HandDetectionCalculator], ERROR!!!");
                    // result.lhand_valid = false;
                }

                if (result.images_rhand_rects.size() == 2 && result.images_rhand_rects[0].size() > 0 && result.images_rhand_rects[1].size() > 0) {
                    // result.rhand_valid = true;

                    auto lcam_rhand = result.images_rhand_rects[0][0];
                    AISDK_LOG_TRACE("[HandDetectionCalculator] right hand + left cam: x: {}, y: {}, w: {}, h: {}", lcam_rhand.x, lcam_rhand.y, lcam_rhand.w, lcam_rhand.h);
                    auto rcam_rhand = result.images_rhand_rects[1][0];
                    AISDK_LOG_TRACE("[HandDetectionCalculator] right hand + right cam: x: {}, y: {}, w: {}, h: {}", rcam_rhand.x, rcam_rhand.y, rcam_rhand.w, rcam_rhand.h);
                } else {
                    // result.rhand_valid = false;
                }

                _ZN2NR200TK7FUNC003E(basealgo);
            }
        }
        break;
    }

    _ZN2NR200TK7FUNC008E(tar_handle);
}
