#include <absl/status/status.h>
#include <opencv2/core/hal/interface.h>

#include <Eigen/Dense>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "aisdk/algorithm/calculator/hand_landmark_calculator.pb.h"
#include "aisdk/algorithm/common/bbox.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/perspective_crop.h"
#include "aisdk/algorithm/func/warpaffine.h"
#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/model/calculator_basenet.h"
#include "aisdk/algorithm/model/hand_rsntiny_artosyn.h"
#include "aisdk/algorithm/model/hand_rtmtiny.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/base/type.h"
#include "aisdk/xengine/cv/xr_cv.h"
#include "aisdk/xgraph/xgraph.h"
#include "xgraph_service_utils.h"

namespace aisdk::algorithm {

// A calculator generate hand landmark result, based on rsntiny/rsnnano neural network.
// Definition:
// node {
//   calculator: "HandLandmarkBatchCalculator"
//   input_stream: "BBOX_SMOOTHED_OUTPUT:detection_smoothed_output"
//   input_stream: "IMAGE_INPUT:image"
//   output_stream: "LANDMARK_OUTPUT:kpt2d"
//   node_options: {
//   [type.googleapis.com/aisdk.HandLandmarkCalculatorOptions] {
//           input_height: 128
//           input_width: 128
//    }
// }

/// @brief
/// 基于双目摄像头（或单目）输入的图像和检测框，批量处理手部关键点检测，生成手部姿态相关的2D关键点坐标及相对深度信息
class HandLandmarkBatchCalculator : public xgraph::CalculatorBase {
   private:
    // RSNTiny algo instance
    std::shared_ptr<HandLandmarkBaseNet> netalgo;  //模型对象

    int32_t input_width_;  //
    int32_t input_height_;
    std::string model_name_;  //模型名称
    bool pcl_able_;
    float bbox_expand_ratio_ = 1.3;
    float mono_valid_bbox_area_ = 30000;  // 检测框面积阈值
    std::shared_ptr<base::BaseCameraModel> lcam_model_ = nullptr;
    std::shared_ptr<base::BaseCameraModel> rcam_model_ = nullptr;
    enum class CropMethod { WarpAffine, PCL };

   public:
    /// @brief 设置calculator的输入输出关系和对应数据类型
    /// @param cc mediapipe计算图的上下文（提供输出输出流，SidePacket，选项参数等）
    /// @return absl::OkStatus()
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] GetContract start");

        /**
         * 输入：
         * BBOX_SMOOTHED_OUTPUT：平滑后的检测框
         * IMAGE_INPUT：图像
         * CAM_INFO_INPUT：相机参数（静态数据包）
         */
        cc->Inputs().Tag("IMAGE_INPUT").Set<std::vector<Image>>();
        cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").Set<DetOutputInternal>();
        cc->InputSidePackets().Tag("CAM_INFO_INPUT").Set<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>();

        /**
         * 输出：
         * LANDMARK_OUTPUT：2D关键点
         */
        cc->Outputs().Tag("LANDMARK_OUTPUT").Set<Kpt2dInternal>();

        AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] GetContract complete");
        return absl::OkStatus();
    }

    /// @brief 加载模型，分配资源，初始化参数（计算节点启动时执行一次）
    /// @param cc mediapipe计算图的上下文（提供输入输出流，SidePacket，选项参数等）
    /// @return 返回结果，成功返回absl::OkStatus()
    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] Open start");
        const auto& options = cc->Options<aisdk::HandLandmarkCalculatorOptions>();
        input_height_ = options.input_height();
        input_width_ = options.input_width();
        model_name_ = options.model_name();
        pcl_able_ = options.pcl_able();  //是否启用PCL剪裁模式
        if (options.bbox_expand_ratio() > 0) {
            bbox_expand_ratio_ = options.bbox_expand_ratio();
        }

        //根据模型名称，寻找适配的模型
        if (model_name_ == "2d_rtmtinyb2") {
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] start init rtmtinyb2");
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<RTMTiny>((void*)0x202310, model_name_);
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] finish init rtmtinyb2");
        } else if ("2d_rtmtiny_ar9481npu_gina" == model_name_) {
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] start init artosyn_rsn_tiny");
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<ArtosynRSNTiny>((void*)0x202310, model_name_);
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] finish init artosyn_rsn_tiny");
        } else {
            AISDK_LOG_ERROR("HandDetTrackCalculator init failed. can not find model:{}", model_name_);
            return absl::AbortedError(fmt::format("can not init model with {}", model_name_));
        }

        if (!netalgo) {
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator]  init landmark model failed");
            return {absl::StatusCode::kInvalidArgument,
                    "[HandLandmarkBatchCalculator] CreateNetAlgoBase nodename error"};
        }

        //解析加载输入的相机相关信息
        const auto& cam_info = cc->InputSidePackets()
                                   .Tag("CAM_INFO_INPUT")
                                   .Get<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        lcam_model_ = cam_info.at(0);
        if (cam_info.size() == 2) {
            rcam_model_ = cam_info.at(1);
        }

        AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] Open complete");
        return absl::OkStatus();
    }

    [[nodiscard]] Vec4f_t GetCropBboxShape(const DetectRect& bbox, float scale) const {
        Vec4f_t bbox_xywh{bbox.x, bbox.y, bbox.w, bbox.h};
        Vec4f_t bbox_cs = bbox_xywh2cs(bbox_xywh);
        bbox_cs.block<2, 1>(2, 0) *= scale;
        auto max_shape = std::max(bbox_cs[2], bbox_cs[3]);
        bbox_cs[2] = max_shape;
        bbox_cs[3] = max_shape;
        return bbox_cs;  // center.x center.y + 扩为方形框的边长
    }

    /// @brief 处理单只手部检测结果
    /// @param image_data 输入图像数据
    /// @param bbox 检测到的手部边界框
    /// @param left_hand 是否为左手的标志
    /// @param origin_camera 原始相机模型
    /// @param kpt 输出关键点坐标
    /// @param rdepth 输出相对深度信息
    /// @param virutal_camera 生成的虚拟相机模型
    /// @param det_flag 检测标志（未在函数中使用）
    /// @return
    absl::Status ProcessSingleHand(const Image& image_data, const DetectRect& bbox, bool left_hand,
                                   base::BaseCameraModel* origin_camera, std::vector<Vec2f_t>& kpt,
                                   std::vector<float>& rdepth,
                                   std::shared_ptr<base::PerspectiveCameraModel>& virutal_camera, bool det_flag) {
        // step1: 准备裁剪图像
        cv::Mat crop_image;

        // step2: 计算边界框扩展比例
        float bbox_scale = bbox_expand_ratio_;

        // step3: 获取扩展后的边界框形状
        Vec4f_t rect = GetCropBboxShape(bbox, bbox_scale);

        // step4: 获取原始相机参数
        auto K = origin_camera->get_camera_intrinsics();   //内参矩阵
        auto kc = origin_camera->get_distortion_params();  //畸变参数

        // step5: 根据边界框生成虚拟相机模型
        virutal_camera = GetVirtualCameraFromBox(origin_camera, rect, {input_width_, input_height_});

        // step6: 平台特定图像剪裁（仅限Android ARM64）
#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
        if (!(image_data.m_mat.empty())) {
            crop_image = xengine::perspective_crop_image_raw(origin_camera, virutal_camera.get(), input_width_,
                                                             input_height_, image_data.m_mat);
        } else {
            AISDK_LOG_ERROR("ProcessSingleHand failed. image_data.m_mat is empty");
            return {absl::StatusCode::kInternal,
                    "[HandLandmarkBatchCalculator] ProcessSingleHand failed. image_data.m_mat is empty."};
        }
#endif

        // step7: 镜像处理左手数据
        if (left_hand) {
            cv::flip(crop_image, crop_image, 1);  //水平翻转
        }

        // step8: 执行神经网络推理
        auto rsn_result = netalgo->Inference({crop_image, crop_image});
        if (!rsn_result.ok()) {
            AISDK_LOG_ERROR("[HandLandmarkBatchCalculator] netalgo->Inference failed");
            return rsn_result.status();
        }

        // step9: 处理关键点坐标
        if (left_hand) {
            //镜像翻转关键点坐标
            std::transform(rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(), kpt.begin(), [&](const auto& kpt) {
                return Vec2f_t{input_width_ - 1 - kpt[0], kpt[1]};
            });
        } else {
            //直接使用原始关键点
            kpt = rsn_result->kpts[0];
        }

        // step10: 处理深度信息
        if (!rsn_result->rdepths.empty()) {
            std::copy(rsn_result->rdepths[0].begin(), rsn_result->rdepths[0].end(), rdepth.begin());
        }

        //返回成功状态
        return absl::OkStatus();
    }

    /// @brief 批量处理双手检测结果（适用于双目摄像头系统）
    /// @param image_data 双目图像输入【左图，右图】
    /// @param bboxes 对应的检测框【左框，右框】
    /// @param left_hand 是否为左手的标志
    /// @param crop_method 裁剪方式（仿射变换/透视变换）
    /// @param lcam_model 左相机模型
    /// @param rcam_model 右相机模型
    /// @param kpt_lcam 输出左相机坐标系关键点
    /// @param kpt_rcam 输出右相机坐标系关键点
    /// @param rdepth_lcam 输出左相机深度信息
    /// @param rdepth_rcam 输出右相机深度信息
    /// @param lvirutal_camera 生成的虚拟左相机模型
    /// @param rvirutal_camera 生成的虚拟右相机模型
    /// @return
    absl::Status ProcessBatchHand(const std::vector<Image>& image_data, const std::vector<DetectRect>& bboxes,
                                  bool left_hand, CropMethod crop_method, base::BaseCameraModel* lcam_model,
                                  base::BaseCameraModel* rcam_model, std::vector<Vec2f_t>& kpt_lcam,
                                  std::vector<Vec2f_t>& kpt_rcam, std::vector<float>& rdepth_lcam,
                                  std::vector<float>& rdepth_rcam,
                                  std::shared_ptr<base::PerspectiveCameraModel>& lvirutal_camera,
                                  std::shared_ptr<base::PerspectiveCameraModel>& rvirutal_camera) {
        // 参数初始化
        float bbox_scale = bbox_expand_ratio_;  // 边界框扩展比例
        std::vector<Image> crop_images(2);      // 存储左右裁剪图像
        std::vector<Vec4f_t> rects(2);          // 存储扩展后的边界框

        // 循环处理左右摄像头数据
        for (int i = 0; i < 2; i++) {
            const auto& bbox = bboxes[i];

            // step1: 计算扩展后的边界框
            rects[i] = GetCropBboxShape(bbox, bbox_scale);  // [x, y, w, h]格式
            cv::Mat crop_image;

            // step2: 根据裁剪方法选择处理方式
            if (crop_method == CropMethod::WarpAffine) {
                //方式1: 仿射变换裁剪
                crop_image = generate_roi_image(image_data[i].m_mat, rects[i], input_width_, input_height_);
            } else {
                //方式2: 透视变换裁剪（需虚拟相机）
                if (i == 0) {  //左摄像头处理
                    lvirutal_camera = GetVirtualCameraFromBox(lcam_model, rects[0], {input_width_, input_height_});
#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
                    // Android平台专用裁剪实现
                    crop_image = xengine::perspective_crop_image(lcam_model_.get(), lvirutal_camera.get(), input_width_,
                                                                 input_height_, image_data[0].m_mat);
#endif
                } else {  //右摄像头处理
                    rvirutal_camera = GetVirtualCameraFromBox(rcam_model, rects[1], {input_width_, input_height_});
#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
                    crop_image = xengine::perspective_crop_image(rcam_model_.get(), rvirutal_camera.get(), input_width_,
                                                                 input_height_, image_data[1].m_mat);
#endif
                }
            }

            // step3: 左手操作镜像处理
            if (left_hand) {
                cv::flip(crop_image, crop_image, 1);  // 水平翻转
            }
            crop_images[i] = crop_image;
        }

        // step4: 执行神经网络推理
        auto rsn_result = netalgo->Inference(crop_images);
        AISDK_LOG_TRACE("[LiftCalculator] kpt2d output : {} {} {} {}", rsn_result->kpts[0][0][0],
                        rsn_result->kpts[0][0][1], rsn_result->kpts[1][0][0], rsn_result->kpts[1][0][1])
        if (!rsn_result.ok()) {
            return rsn_result.status();
        }

        // step5: 关键点坐标转换
        if (left_hand) {
            if (crop_method == CropMethod::WarpAffine) {
                // 仿射变换 + 左手镜像处理
                // 关键点映射公式：将裁剪图像坐标映射回原图坐标系
                // X' = (镜像X坐标 * 原图宽度比例) + 原图X偏移 - 半宽补偿
                // Y' = (Y坐标 * 原图高度比例) + 原图Y偏移 - 半高补偿

                //这里处理左摄像头
                std::transform(
                    rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(), kpt_lcam.begin(), [&](const auto& kpt) {
                        return Vec2f_t{
                            (input_width_ - 1 - kpt[0]) * rects[0][2] / input_width_ + rects[0][0] - rects[0][2] * 0.5,
                            (kpt[1]) * rects[0][3] / input_height_ + rects[0][1] - rects[0][3] * 0.5};
                    });

                //这里处理右摄像头
                std::transform(
                    rsn_result->kpts[1].begin(), rsn_result->kpts[1].end(), kpt_rcam.begin(), [&](const auto& kpt) {
                        return Vec2f_t{
                            (input_width_ - 1 - kpt[0]) * rects[1][2] / input_width_ + rects[1][0] - rects[1][2] * 0.5,
                            (kpt[1]) * rects[1][3] / input_height_ + rects[1][1] - rects[1][3] * 0.5};
                    });
            } else {
                //透视变换直接镜像处理
                std::transform(rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(), kpt_lcam.begin(),
                               [&](const auto& kpt) {
                                   return Vec2f_t{input_width_ - 1 - kpt[0], kpt[1]};
                               });
                std::transform(rsn_result->kpts[1].begin(), rsn_result->kpts[1].end(), kpt_rcam.begin(),
                               [&](const auto& kpt) {
                                   return Vec2f_t{input_width_ - 1 - kpt[0], kpt[1]};
                               });
            }
        } else {  //非左手操作处理
            if (crop_method == CropMethod::WarpAffine) {
                //仿射变换无镜像
                std::transform(
                    rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(), kpt_lcam.begin(), [&](const auto& kpt) {
                        return Vec2f_t{kpt[0] * rects[0][2] / input_width_ + rects[0][0] - rects[0][2] * 0.5,
                                       (kpt[1]) * rects[0][3] / input_height_ + rects[0][1] - rects[0][3] * 0.5};
                    });
                std::transform(
                    rsn_result->kpts[1].begin(), rsn_result->kpts[1].end(), kpt_rcam.begin(), [&](const auto& kpt) {
                        return Vec2f_t{kpt[0] * rects[1][2] / input_width_ + rects[1][0] - rects[1][2] * 0.5,
                                       (kpt[1]) * rects[1][3] / input_height_ + rects[1][1] - rects[1][3] * 0.5};
                    });
            } else {
                // 透视变换直接使用原始坐标
                std::transform(rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(), kpt_lcam.begin(),
                               [&](const auto& kpt) {
                                   return Vec2f_t{kpt[0], kpt[1]};
                               });
                std::transform(rsn_result->kpts[1].begin(), rsn_result->kpts[1].end(), kpt_rcam.begin(),
                               [&](const auto& kpt) {
                                   return Vec2f_t{kpt[0], kpt[1]};
                               });
            }
        }

        // step6: 深度信息处理
        if (!rsn_result->rdepths.empty()) {
            std::copy(rsn_result->rdepths[0].begin(), rsn_result->rdepths[0].end(), rdepth_lcam.begin());
            std::copy(rsn_result->rdepths[1].begin(), rsn_result->rdepths[1].end(), rdepth_rcam.begin());
        }

        return absl::OkStatus();
    }

    /// @brief 根据模式参数，对双摄像头手部检测数据实施过滤
    /// @param cc mediapipe计算图的上下文
    /// @return absl::OkStatus()
    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(HandLandmarkBatchCalculator::Process);
#endif
        //输入标准检查
        if (cc->Inputs().Tag("IMAGE_INPUT").IsEmpty() || cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").IsEmpty()) {
            AISDK_LOG_TRACE(
                "[HandLandmarkBatchCalculator] IMAGE_INPUT/BBOX_SMOOTHED_OUTPUT lost, this loop terminated here!");
            return absl::OkStatus();
        }

        //获取输入数据
        const auto& image_data = cc->Inputs().Tag("IMAGE_INPUT").Get<std::vector<Image>>();
        const auto& bbox_data = cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").Get<DetOutputInternal>();
        std::unique_ptr<Kpt2dInternal> output_buffer_ = absl::make_unique<Kpt2dInternal>();
        CropMethod crop_method = CropMethod::WarpAffine;
        if (pcl_able_) {
            crop_method = CropMethod::PCL;
        }

        AISDK_LOG_TRACE(
            "[HandLandmarkBatchCalculator] input bbox_data: lhand_lcam_valid[{}], lhand_rcam_valid[{}], "
            "rhand_lcam_valid[{}], rhand_rcam_valid[{}], image_data.size[{}]",
            bbox_data.lhand_lcam_valid, bbox_data.lhand_rcam_valid, bbox_data.rhand_lcam_valid,
            bbox_data.rhand_rcam_valid, image_data.size());

        /**
         * 双目处理逻辑：
         * lhand_lcam和lhan_rcam都存在，则认为是单目的左手;
         * rhand_lcam和rhand_rcam都存在，则认为是单目的右手;
         */
        if (bbox_data.lhand_lcam_valid && bbox_data.lhand_rcam_valid) {
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] bino left_hand");
            auto result = ProcessBatchHand(
                image_data, {bbox_data.lhand_lcam_rect, bbox_data.lhand_rcam_rect}, true, crop_method,
                lcam_model_.get(), rcam_model_.get(), output_buffer_->lhand_lcam_kpt, output_buffer_->lhand_rcam_kpt,
                output_buffer_->lhand_lcam_rdepth, output_buffer_->lhand_rcam_rdepth,
                output_buffer_->lhand_lcam_virtual_camera, output_buffer_->lhand_rcam_virtual_camera);
            if (result.ok()) {
                output_buffer_->lhand_lcam_valid = true;
                output_buffer_->lhand_rcam_valid = true;
            }
        }
        if (bbox_data.rhand_lcam_valid && bbox_data.rhand_rcam_valid) {
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] bino right_hand");
            auto result = ProcessBatchHand(
                image_data, {bbox_data.rhand_lcam_rect, bbox_data.rhand_rcam_rect}, false, crop_method,
                lcam_model_.get(), rcam_model_.get(), output_buffer_->rhand_lcam_kpt, output_buffer_->rhand_rcam_kpt,
                output_buffer_->rhand_lcam_rdepth, output_buffer_->rhand_rcam_rdepth,
                output_buffer_->rhand_lcam_virtual_camera, output_buffer_->rhand_rcam_virtual_camera);
            if (result.ok()) {
                output_buffer_->rhand_lcam_valid = true;
                output_buffer_->rhand_rcam_valid = true;
            }
        }

        /**
         * 单目处理逻辑(单目只集中在相机lcam上，并且由于只有一张图，所以只处理image_data[0])：
         * lhand_lcam存在，并且lhand_ram不存在，则认为是单目的左手
         * rhand_lcam存在，并且rhand_rcam不存在，则认为是单目的右手
         */
        if (bbox_data.lhand_lcam_valid && !bbox_data.lhand_rcam_valid) {
            float bbox_area = bbox_data.lhand_lcam_rect.w * bbox_data.lhand_lcam_rect.h;
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] mono left_hand bbox_area {}", bbox_area);
            auto result = ProcessSingleHand(image_data[0], bbox_data.lhand_lcam_rect, true, lcam_model_.get(),
                                            output_buffer_->lhand_lcam_kpt, output_buffer_->lhand_lcam_rdepth,
                                            output_buffer_->lhand_lcam_virtual_camera, bbox_data.det_flag);
            if (result.ok() && bbox_area < mono_valid_bbox_area_) {
                output_buffer_->lhand_lcam_valid = true;
                output_buffer_->lhand_rcam_valid = false;
            } else {
                AISDK_LOG_ERROR(
                    "[HandLandmarkBatchCalculator] mono left_hand failed. bbox_area[{}] < mono_valid_bbox_area_[{}] or "
                    "result.ok() is false. w[{}], h[{}]",
                    bbox_area, mono_valid_bbox_area_, bbox_data.lhand_lcam_rect.w, bbox_data.lhand_lcam_rect.h);
            }
        }
        if (bbox_data.rhand_lcam_valid && !bbox_data.rhand_rcam_valid) {
            float bbox_area = bbox_data.rhand_lcam_rect.w * bbox_data.rhand_lcam_rect.h;
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] mono right_hand bbox_area {}", bbox_area);
            auto result = ProcessSingleHand(image_data[0], bbox_data.rhand_lcam_rect, false, lcam_model_.get(),
                                            output_buffer_->rhand_lcam_kpt, output_buffer_->rhand_lcam_rdepth,
                                            output_buffer_->rhand_lcam_virtual_camera, bbox_data.det_flag);
            if (result.ok() && bbox_area < mono_valid_bbox_area_) {
                output_buffer_->rhand_lcam_valid = true;
                output_buffer_->rhand_rcam_valid = false;
            } else {
                AISDK_LOG_ERROR(
                    "[HandLandmarkBatchCalculator] mono right_hand failed. bbox_area[{}] < mono_valid_bbox_area_[{}] "
                    "or result.ok() is false. w[{}], h[{}]",
                    bbox_area, mono_valid_bbox_area_, bbox_data.rhand_lcam_rect.w, bbox_data.rhand_lcam_rect.h);
            }
        }

        //输出
        if (output_buffer_->lhand_lcam_valid || output_buffer_->lhand_rcam_valid || output_buffer_->rhand_lcam_valid ||
            output_buffer_->rhand_rcam_valid) {
            // clang-format off
            AISDK_LOG_TRACE(
                "[HandLandmarkBatchCalculator] lhand_lcam_valid: {}, lhand_rcam_valid: {},  lhand_lcam: {}, lhand_rcam: {} / rhand_lcam_valid: {}, rhand_rcam_valid: {},  rhand_lcam: {}, rhand_rcam: {}",
                output_buffer_->lhand_lcam_valid, output_buffer_->lhand_rcam_valid, output_buffer_->lhand_lcam_kpt.size(), output_buffer_->lhand_rcam_kpt.size(),
                output_buffer_->rhand_lcam_valid, output_buffer_->rhand_rcam_valid, output_buffer_->rhand_lcam_kpt.size(), output_buffer_->rhand_rcam_kpt.size());
            cc->Outputs().Tag("LANDMARK_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            // clang-format on
        } else {
            cc->Outputs().Tag("LANDMARK_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] Process complete");
        return absl::OkStatus();
    }
};
};  // namespace aisdk::algorithm
