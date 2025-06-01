#include <memory>

#include "../common/NR_GlobalPredictorService.h"
#include "../common/NR_Seq_Manager.h"
#include "../internal_structs/det_struct_internal.h"
#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"

namespace aisdk::algorithm {

// A calculator generate smoothed detection bbox result.
// Definition:
// node {
//   calculator: "DetectBoxSmoothingCalculator"
//   input_stream: "BBOX_INPUT:detection_output"
//   output_stream: "BBOX_SMOOTHED_OUTPUT:detection_smoothed_output"
// }

/// @brief 检测框平滑类（对输入的手部检测框进行平滑滤波处理，输出更稳定的边界框）
class DetectBoxSmoothingCalculator : public xgraph::CalculatorBase {
   private:
    std::shared_ptr<SeqManager> m_seq_lcam_lhand;
    std::shared_ptr<SeqManager> m_seq_lcam_rhand;
    std::shared_ptr<SeqManager> m_seq_rcam_lhand;
    std::shared_ptr<SeqManager> m_seq_rcam_rhand;

   public:
    /// @brief 设置calculator的输入输出关系和对应数据类型
    /// @param cc mediapipe计算图的上下文（提供输出输出流，SidePacket，选项参数等）
    /// @return absl::OkStatus()
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] GetContract start");

        cc->Inputs().Tag("BBOX_INPUT").Set<DetOutputInternal>();
        cc->Outputs().Tag("BBOX_SMOOTHED_OUTPUT").Set<DetOutputInternal>();

        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] GetContract complete");
        return absl::OkStatus();
    }

    /// @brief 加载模型，分配资源，初始化参数（计算节点启动时执行一次）
    /// @param cc mediapipe计算图的上下文（提供输入输出流，SidePacket，选项参数等）
    /// @return 返回结果，成功返回absl::OkStatus()
    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Open start");

        m_seq_lcam_lhand = std::make_shared<SeqManager>();
        m_seq_lcam_rhand = std::make_shared<SeqManager>();
        m_seq_rcam_lhand = std::make_shared<SeqManager>();
        m_seq_rcam_rhand = std::make_shared<SeqManager>();

        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Open complete");
        return absl::OkStatus();
    }

    /// @brief 对输入的检测框（手部位置）进行平滑滤波处理，输出更稳定的边界框
    /// @param cc mediapipe计算图的上下文
    /// @return absl::OkStatus()
    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(DetectBoxSmoothingCalculator::Process);
#endif

        //获取输入数据，初始化输出缓冲区
        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Process start");
        const auto& input_data = cc->Inputs().Tag("BBOX_INPUT").Get<DetOutputInternal>();
        std::unique_ptr<DetOutputInternal> output_buffer_ = absl::make_unique<DetOutputInternal>();
        *output_buffer_ = input_data;

        //根据历史关键点数据重置滤波器
        const auto kpt3d_world_pre = GlobalPredictorService::getInstance().get_last_kpt3d_world();
        if (!kpt3d_world_pre.lhand_valid) {
            m_seq_lcam_lhand->reset();  //历史数据中无左手，则重置左手相关的滤波器
            m_seq_rcam_lhand->reset();
        }

        if (!kpt3d_world_pre.rhand_valid) {
            m_seq_lcam_rhand->reset();  //历史数据中无右手，则重置右手相关的滤波器
            m_seq_rcam_rhand->reset();
        }

        //分别针对lhand_lcam, lhand_rcam, rhand_lcam, rhand_rcam进行平滑处理
        if (input_data.lhand_lcam_valid) {
            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Do smoothing on lhand lcam bboxes");
            float p_score = 1.0f;
            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] origin w[{}], h[{}]", output_buffer_->lhand_lcam_rect.w,
                            output_buffer_->lhand_lcam_rect.h);
            m_seq_lcam_lhand->getFilterBoxData(output_buffer_->lhand_lcam_rect, p_score);

            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Done smoothing on lhand lcam bboxes");
        }

        if (input_data.lhand_rcam_valid) {
            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Do smoothing on lhand rcam bboxes");
            float p_score = 1.0f;
            m_seq_rcam_lhand->getFilterBoxData(output_buffer_->lhand_rcam_rect, p_score);

            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Done smoothing on lhand rcam bboxes");
        }

        if (input_data.rhand_lcam_valid) {
            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Do smoothing on rhand lcam bboxes");
            float p_score = 1.0f;
            m_seq_lcam_rhand->getFilterBoxData(output_buffer_->rhand_lcam_rect, p_score);

            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Done smoothing on rhand lcam bboxes");
        }

        if (input_data.rhand_rcam_valid) {
            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Do smoothing on rhand rcam bboxes");
            float p_score = 1.0f;
            m_seq_rcam_rhand->getFilterBoxData(output_buffer_->rhand_rcam_rect, p_score);

            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Done smoothing on rhand rcam bboxes");
        }

        //输出结果
        if (output_buffer_->lhand_lcam_valid || output_buffer_->lhand_rcam_valid || output_buffer_->rhand_lcam_valid ||
            output_buffer_->rhand_rcam_valid) {
            cc->Outputs().Tag("BBOX_SMOOTHED_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("BBOX_SMOOTHED_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Process complete");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
