#include <fmt/format.h>

#include <cstdint>
#include <memory>
#include <opencv2/core/types.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

#include "aisdk/algorithm/calculator/hand_detection_track_calculator.pb.h"
#include "aisdk/algorithm/common/NR_GlobalPredictorService.h"
#include "aisdk/algorithm/common/NR_Transfer.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/common/metrics.h"
#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/algorithm/internal_structs/headpose_struct_internal.h"
#include "aisdk/algorithm/model/hand_detect.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
#include "xgraph_service_utils.h"

// TODO: update codes in batch=1 branch
namespace aisdk::algorithm {

// A calculator generate bbox detection result, based on detnet inference result.
// Definition:
// node {
//   name: "HandDetection"
//   calculator: "HandDetTrackCalculator"
//   input_stream: "IMAGE_INPUT:image"
//   output_stream: "DET_BBOX_OUTPUT:detection_output"
// }

/// @brief 手部检测跟踪的calculator类
class HandDetTrackCalculator : public xgraph::CalculatorBase {
   private:
    // DetNet algo instance
    std::shared_ptr<HandDetectNet> netalgo;  //创建的模型对象

    std::shared_ptr<aisdk::base::BaseCameraModel> lcam_model_ = nullptr;
    std::shared_ptr<aisdk::base::BaseCameraModel> rcam_model_ = nullptr;

    std::string model_name_;  //模型名称，calculator配置中设置了此值
    bool is_mono_ = true;     //是否单目，true表示单目
    uint32_t video_width_;
    uint32_t video_height_;
    float min_bbox_area_th_ = 10 * 10;
    int det_tracker_step_ = 0;  //检测和跟踪的计数器
    int det_interval_ = 4;
    float valid_bbox_in_image_ratio_ = 0.8;  // 检测框有效面积比例
    bool enable_track = true;                //是否启用手部跟踪，true表示启用

   public:
    /// @brief 设置手部检测跟踪calculator的输入输出关系和对应数据类型
    /// @param cc mediapipe计算图的上下文（提供输出输出流，SidePacket，选项参数等）
    /// @return 返回结果，成功返回absl::OkStatus()
    static absl::Status GetContract(xgraph::CalculatorContract *cc) {
        AISDK_LOG_TRACE("[HandDetTrackCalculator] GetContract start");

        // Declaration of input and output, according to definitons.
        cc->InputSidePackets().Tag("CAM_INFO_INPUT").Set<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>();

        cc->Inputs().Tag("IMAGE_INPUT").Set<std::vector<Image>>();
        cc->Inputs().Tag("HEADPOSE").Set<HeadPoseInternal>();

        cc->Outputs().Tag("DET_BBOX_OUTPUT").Set<DetOutputInternal>();
        cc->Outputs().Tag("IMAGE_OUTPUT").Set<std::vector<Image>>();
        cc->Outputs().Tag("HEADPOSE_OUTPUT").Set<HeadPoseInternal>();

        AISDK_LOG_TRACE("[HandDetTrackCalculator] GetContract complete");
        return absl::OkStatus();
    }

    /// @brief 加载模型，分配资源，初始化参数（计算节点启动时执行一次）
    /// @param cc mediapipe计算图的上下文（提供输入输出流，SidePacket，选项参数等）
    /// @return 返回结果，成功返回absl::OkStatus()
    absl::Status Open(xgraph::CalculatorContext *cc) final {
        AISDK_LOG_TRACE("[HandDetTrackCalculator] Open start");

        //根据模型名称，选择创建老模型HandDetectNet对象还是新模型HandDetectNetv2对象，根据配置里面，目前版本创建的是新模型
        const auto &options = cc->Options<aisdk::HandDetTrackCalculatorOptions>();
        model_name_ = options.model_name();
        if (model_name_ == "detect_cpu_ella" || model_name_ == "detect_cpu_flora" || model_name_ == "detect_dsp_ella") {
            // 老模型
            AISDK_LOG_TRACE("[HandDetTrackCalculator] HandDetectNet init {}", model_name_);
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<HandDetectNet>((void *)0x202310, model_name_);
        } else {
            // 新模型
            AISDK_LOG_TRACE("[HandDetTrackCalculator] HandDetectNetv2 init {}", model_name_);
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<HandDetectNetv2>((void *)0x202310, model_name_);
        }

        if (!netalgo) {
            return {absl::StatusCode::kInvalidArgument, "[HandDetTrackCalculator] CreateNetAlgoBase nodename error"};
        }

        //读取检测间隔（帧率）
        if (options.det_interval() > 0) {
            det_interval_ = options.det_interval();
        }
        enable_track = options.enable_track();

        //读取摄像头参数，如果有两个摄像头，则读取双目摄像头参数
        const auto &cam_info = cc->InputSidePackets()
                                   .Tag("CAM_INFO_INPUT")
                                   .Get<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        lcam_model_ = cam_info.at(0);
        if (cam_info.size() == 2) {
            rcam_model_ = cam_info.at(1);
            is_mono_ = false;
        }

        //读取摄像头的分辨率
        video_width_ = lcam_model_->video_width_;
        video_height_ = lcam_model_->video_height_;

        //初始化检测和跟踪的计数器
        det_tracker_step_ = 0;

        AISDK_LOG_TRACE("[HandDetTrackCalculator] Open complete.");
        return absl::OkStatus();
    }

    /// @brief 在视频流中检测和跟踪手部位置信息，在每一帧图像中执行检测或跟踪操作
    /// @param cc mediapipe计算图的上下文（提供输出输出流，SidePacket，选项参数等）
    /// @return 返回结果，成功返回absl::OkStatus()
    absl::Status Process(xgraph::CalculatorContext *cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(HandDetTrackCalculator::Process);
#endif
        AISDK_LOG_TRACE("[HandDetTrackCalculator] Process start");

        //获取当前帧的图像数据和手部姿态信息
        const auto &image_data = cc->Inputs().Tag("IMAGE_INPUT").Value().Get<std::vector<Image>>();
        const auto &headpose_data = cc->Inputs().Tag("HEADPOSE").Get<HeadPoseInternal>();

        const auto &timestamp = cc->InputTimestamp().Seconds();

        //获取上一帧的2d, 3d手部关键点信息
        auto lastframe_kpt2d = GlobalPredictorService::getInstance().get_last_kpt2d_pixel();
        auto lastframe_kpt3d = GlobalPredictorService::getInstance().get_last_kpt3d_world();

        std::unique_ptr<DetOutputInternal> output_buffer_ = absl::make_unique<DetOutputInternal>();
        output_buffer_->clear();
        output_buffer_->det_flag = true;

        //如果headpose无效，则停止追踪，当前帧不进行处理
        if (!isHeadPoseValid(headpose_data.transform)) {
            AISDK_LOG_ERROR("[HandDetTrackCalculator] HeadPose isn't valid !!!");
            // headpose异常，停止tracker，并且此帧不分析。
            det_tracker_step_ = 0;
            output_buffer_->det_flag = false;
        }

        auto &predictor_lhand_lcam = GlobalPredictorService::getInstance().get_predictor_lhand_lcam_bbox();
        auto &predictor_rhand_lcam = GlobalPredictorService::getInstance().get_predictor_rhand_lcam_bbox();
        auto &predictor_lhand_rcam = GlobalPredictorService::getInstance().get_predictor_lhand_rcam_bbox();
        auto &predictor_rhand_rcam = GlobalPredictorService::getInstance().get_predictor_rhand_rcam_bbox();

        if ((det_tracker_step_ != 0) && enable_track &&
            (predictor_lhand_lcam.get_tracking_status() || predictor_rhand_lcam.get_tracking_status() ||
             predictor_lhand_rcam.get_tracking_status() || predictor_rhand_rcam.get_tracking_status())) {
            AISDK_LOG_TRACE("[HandDetTrackCalculator] 2D Tracker Starting");

            //处理左相机拍到的左手数据
            if (lastframe_kpt2d.lhand_lcam_valid && lastframe_kpt3d.lhand_valid) {
                DetectRect proj_bbox_lhand_lcam;
                std::vector<Vec2f_t> lhand_lcam_kpt = lastframe_kpt3d.left_hand.kpt2d_lcam;
                Vec2f_t root_kf_predicted;
                Vec2f_t root_meas = lhand_lcam_kpt[kKeypoint2dRootId];

                //根据时间戳等信息，获取预测的根节点位置
                root_kf_predicted = predictor_lhand_lcam.track_only_pred(timestamp, true);

                //以预测的根节点替换原来的根节点，更新所有坐标信息
                for (int k = 0; k < lhand_lcam_kpt.size(); k++) {
                    lhand_lcam_kpt[k] = lhand_lcam_kpt[k] + root_kf_predicted - root_meas;
                }

                //计算出检测框，检测框有效，则存入output_buffer_->lhand_lcam_rect
                proj_bbox_lhand_lcam = kpts_to_bbox(lhand_lcam_kpt);
                if (check_if_rect_valid(proj_bbox_lhand_lcam, video_width_, video_height_, valid_bbox_in_image_ratio_,
                                        min_bbox_area_th_)) {
                    output_buffer_->lhand_lcam_valid = true;
                    output_buffer_->lhand_lcam_rect = proj_bbox_lhand_lcam;
                }
            }

            //处理左相机拍到的右手
            if (lastframe_kpt2d.rhand_lcam_valid && lastframe_kpt3d.rhand_valid) {
                DetectRect proj_bbox_rhand_lcam;
                std::vector<Vec2f_t> rhand_lcam_kpt = lastframe_kpt3d.right_hand.kpt2d_lcam;
                Vec2f_t root_kf_predicted;
                Vec2f_t root_meas = rhand_lcam_kpt[kKeypoint2dRootId];

                //根据时间戳，获取预测的根节点位置
                root_kf_predicted = predictor_rhand_lcam.track_only_pred(timestamp, true);

                //以预测的根节点替换原来的根节点，并更新其他节点坐标
                for (int k = 0; k < rhand_lcam_kpt.size(); k++) {
                    rhand_lcam_kpt[k] = rhand_lcam_kpt[k] + root_kf_predicted - root_meas;
                }

                //计算出检测框，检测框有效，则存入output_buffer_->rhand_lcam_rect
                proj_bbox_rhand_lcam = kpts_to_bbox(rhand_lcam_kpt);
                if (check_if_rect_valid(proj_bbox_rhand_lcam, video_width_, video_height_, valid_bbox_in_image_ratio_,
                                        min_bbox_area_th_)) {
                    output_buffer_->rhand_lcam_valid = true;
                    output_buffer_->rhand_lcam_rect = proj_bbox_rhand_lcam;
                }
                // 单目流, track不会出右目的框
            }

            //处理右相机拍到的左手
            if (lastframe_kpt2d.lhand_rcam_valid && lastframe_kpt3d.lhand_valid) {
                DetectRect proj_bbox_lhand_rcam;
                std::vector<Vec2f_t> lhand_rcam_kpt = lastframe_kpt3d.left_hand.kpt2d_rcam;
                Vec2f_t root_kf_predicted;
                Vec2f_t root_meas = lhand_rcam_kpt[kKeypoint2dRootId];

                //根据时间戳，获取预测的根节点位置
                root_kf_predicted = predictor_lhand_rcam.track_only_pred(timestamp, true);

                //以预测的根节点替换原来的根节点，并更新其他坐标节点
                for (int k = 0; k < lhand_rcam_kpt.size(); k++) {
                    lhand_rcam_kpt[k] = lhand_rcam_kpt[k] + root_kf_predicted - root_meas;
                }

                //计算出检测框，检测框有效，则存入output_buffer_->lhand_rcam_rect
                proj_bbox_lhand_rcam = kpts_to_bbox(lhand_rcam_kpt);
                if (check_if_rect_valid(proj_bbox_lhand_rcam, video_width_, video_height_, valid_bbox_in_image_ratio_,
                                        min_bbox_area_th_)) {
                    output_buffer_->lhand_rcam_valid = true;
                    output_buffer_->lhand_rcam_rect = proj_bbox_lhand_rcam;
                }
            }

            //处理右相机拍到的右手
            if (lastframe_kpt2d.rhand_rcam_valid && lastframe_kpt3d.rhand_valid) {
                DetectRect proj_bbox_rhand_rcam;
                std::vector<Vec2f_t> rhand_rcam_kpt = lastframe_kpt3d.right_hand.kpt2d_rcam;
                Vec2f_t root_kf_predicted;
                Vec2f_t root_meas = rhand_rcam_kpt[kKeypoint2dRootId];

                //根据时间戳，获取预测的根节点位置
                root_kf_predicted = predictor_rhand_rcam.track_only_pred(timestamp, true);

                //以预测的根节点，替换原来的根节点，并更新其他坐标节点
                for (int k = 0; k < rhand_rcam_kpt.size(); k++) {
                    rhand_rcam_kpt[k] = rhand_rcam_kpt[k] + root_kf_predicted - root_meas;
                }

                //计算出检测框，检测框有效，则存入output_buffer_->rhand_rcam_rect
                proj_bbox_rhand_rcam = kpts_to_bbox(rhand_rcam_kpt);
                if (check_if_rect_valid(proj_bbox_rhand_rcam, video_width_, video_height_, valid_bbox_in_image_ratio_,
                                        min_bbox_area_th_)) {
                    output_buffer_->rhand_rcam_valid = true;
                    output_buffer_->rhand_rcam_rect = proj_bbox_rhand_rcam;
                }
            }

            //有跟踪到手
            if (output_buffer_->lhand_lcam_valid || output_buffer_->lhand_rcam_valid ||
                output_buffer_->rhand_lcam_valid || output_buffer_->rhand_rcam_valid) {
                output_buffer_->det_flag = false;

                det_tracker_step_++;
                if (det_tracker_step_ > det_interval_) {
                    det_tracker_step_ = 0;
                }
            }
        }

        //没有跟踪到手，则通过模型进行检测
        if (output_buffer_->det_flag) {
            // do detection
            auto &result = *output_buffer_;

            // detnet inference
            auto status = netalgo->Inference(image_data, result);
            if (!status.ok()) {
                return status;
            }

            //检查检测的框是否有效
            if (result.images_lhand_rects[0].size() > 0) {
                result.lhand_lcam_rect = result.images_lhand_rects[0][0];
                if (check_if_rect_valid(result.lhand_lcam_rect, video_width_, video_height_, valid_bbox_in_image_ratio_,
                                        min_bbox_area_th_)) {
                    result.lhand_lcam_valid = true;
                }
            }
            if (!is_mono_ && result.images_lhand_rects[1].size() > 0) {
                result.lhand_rcam_rect = result.images_lhand_rects[1][0];
                if (check_if_rect_valid(result.lhand_rcam_rect, video_width_, video_height_, valid_bbox_in_image_ratio_,
                                        min_bbox_area_th_)) {
                    result.lhand_rcam_valid = true;
                }
            }

            if (result.images_rhand_rects[0].size() > 0) {
                result.rhand_lcam_rect = result.images_rhand_rects[0][0];
                if (check_if_rect_valid(result.rhand_lcam_rect, video_width_, video_height_, valid_bbox_in_image_ratio_,
                                        min_bbox_area_th_)) {
                    result.rhand_lcam_valid = true;
                }
            }
            if (!is_mono_ && result.images_rhand_rects[1].size() > 0) {
                result.rhand_rcam_rect = result.images_rhand_rects[1][0];
                if (check_if_rect_valid(result.rhand_rcam_rect, video_width_, video_height_, valid_bbox_in_image_ratio_,
                                        min_bbox_area_th_)) {
                    result.rhand_rcam_valid = true;
                }
            }
            det_tracker_step_ = 1;
        }

        //输出结果
        if (output_buffer_->lhand_lcam_valid || output_buffer_->lhand_rcam_valid || output_buffer_->rhand_lcam_valid ||
            output_buffer_->rhand_rcam_valid) {
            cc->Outputs().Tag("DET_BBOX_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            cc->Outputs().Tag("IMAGE_OUTPUT").AddPacket(cc->Inputs().Tag("IMAGE_INPUT").Value());
            cc->Outputs().Tag("HEADPOSE_OUTPUT").AddPacket(cc->Inputs().Tag("HEADPOSE").Value());
            AISDK_LOG_TRACE("[HandDetTrackCalculator] At least single hand valid, pass");
        } else {
            // update lastframe kpt3d
            AISDK_LOG_ERROR("No valid hand, stop tracking");
            lastframe_kpt2d.lhand_lcam_valid = false;
            lastframe_kpt2d.rhand_lcam_valid = false;
            lastframe_kpt2d.lhand_rcam_valid = false;
            lastframe_kpt2d.rhand_rcam_valid = false;
            auto &predictor_lhand_lcam = GlobalPredictorService::getInstance().get_predictor_lhand_lcam_bbox();
            predictor_lhand_lcam.stop_tracking();
            auto &predictor_rhand_lcam = GlobalPredictorService::getInstance().get_predictor_rhand_lcam_bbox();
            predictor_rhand_lcam.stop_tracking();
            auto &predictor_lhand_rcam = GlobalPredictorService::getInstance().get_predictor_lhand_rcam_bbox();
            predictor_lhand_rcam.stop_tracking();
            auto &predictor_rhand_rcam = GlobalPredictorService::getInstance().get_predictor_rhand_rcam_bbox();
            predictor_rhand_rcam.stop_tracking();
            auto &predictor_lhand = GlobalPredictorService::getInstance().get_predictor_lhand();
            predictor_lhand.stop_tracking();
            auto &predictor_rhand = GlobalPredictorService::getInstance().get_predictor_rhand();
            predictor_rhand.stop_tracking();

            // #if defined(ENABLE_ALGORITHM_DATA_RECORD) && !defined(ENABLE_SEGMENT_JOINT_INFERENCE_MODE)
            cc->Outputs().Tag("DET_BBOX_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            cc->Outputs().Tag("IMAGE_OUTPUT").AddPacket(cc->Inputs().Tag("IMAGE_INPUT").Value());
            cc->Outputs().Tag("HEADPOSE_OUTPUT").AddPacket(cc->Inputs().Tag("HEADPOSE").Value());
            //             AISDK_LOG_TRACE("[HandDetTrackCalculator] empty hand, pass");
            // #else
            AISDK_LOG_TRACE("[HandDetTrackCalculator] No valid hand, truncated here");
            // #endif
        }

        AISDK_LOG_TRACE("[HandDetTrackCalculator] Process complete");

        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
