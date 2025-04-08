#include <memory>
#include <vector>

#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
#include "mono_bino_switch_calculator.pb.h"

namespace aisdk::algorithm {

/// @brief 根据模式参数，对双摄像头手部检测数据实施过滤
class MonoBinoSwitchCalculator : public xgraph::CalculatorBase {
   private:
    std::string mode_ = "BINO";

   public:
    /// @brief 设置calculator的输入输出关系和对应数据类型
    /// @param cc mediapipe计算图的上下文（提供输出输出流，SidePacket，选项参数等）
    /// @return absl::OkStatus()
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[MonoBinoSwitchCalculator] GetContract start");
        cc->Inputs().Tag("BBOX_IN").Set<DetOutputInternal>();
        cc->Outputs().Tag("BBOX_OUT").Set<DetOutputInternal>();
        AISDK_LOG_TRACE("[MonoBinoSwitchCalculator] GetContract finish");
        return absl::OkStatus();
    }

    /// @brief 加载模型，分配资源，初始化参数（计算节点启动时执行一次）
    /// @param cc mediapipe计算图的上下文（提供输入输出流，SidePacket，选项参数等）
    /// @return 返回结果，成功返回absl::OkStatus()
    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[MonoBinoSwitchCalculator] Open start");
        const auto& options = cc->Options<aisdk::MonoBinoSwitchCalculatorOptions>();
        if (!options.mode().empty()) {
            mode_ = options.mode();
        }
        AISDK_LOG_TRACE("[MonoBinoSwitchCalculatorCalculator] Open complete");
        return absl::OkStatus();
    }

    /// @brief 根据模式参数，对双摄像头手部检测数据实施过滤
    /// @param cc mediapipe计算图的上下文
    /// @return absl::OkStatus()
    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(MonoBinoSwitchCalculator::Process);
#endif

        //获取输入数据，初始化输出缓冲区
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Process start");
        auto output_buffer_ = absl::make_unique<DetOutputInternal>();
        const auto& bbox_data = cc->Inputs().Tag("BBOX_IN").Get<DetOutputInternal>();
        *output_buffer_ = bbox_data;

        //根据模式修改输出数据
        if (mode_ == "MONO") {
            // MONO模式，禁用“非主摄像头”数据，只采用左摄像头的左手数据和右摄像头的右手数据
            output_buffer_->rhand_lcam_valid = false;  //右摄像头下的左手数据无效
            output_buffer_->lhand_rcam_valid = false;  //左摄像头下的右手数据无效
        } else if (mode_ == "BINO") {
            // BINO模式，仅当两个摄像头都有某个手时，才有效
            output_buffer_->lhand_lcam_valid = bbox_data.lhand_lcam_valid && bbox_data.lhand_rcam_valid;
            output_buffer_->lhand_rcam_valid = bbox_data.lhand_lcam_valid && bbox_data.lhand_rcam_valid;
            output_buffer_->rhand_lcam_valid = bbox_data.rhand_lcam_valid && bbox_data.rhand_rcam_valid;
            output_buffer_->rhand_rcam_valid = bbox_data.rhand_lcam_valid && bbox_data.rhand_rcam_valid;
        } else if (mode_ == "SWITCH") {
            // SWITCH模式，若主摄像头数据无效，则关闭次摄像头数据
            if (!bbox_data.lhand_lcam_valid) {
                output_buffer_->lhand_rcam_valid = false;  //左摄像头无左手数据时，其拍摄的右手数据也作废
            }
            if (!bbox_data.rhand_rcam_valid) {
                output_buffer_->rhand_lcam_valid = false;  //右摄像头无右手数据时，其拍摄的左手数据也作废
            }
        }

        //输出数据
        cc->Outputs().Tag("BBOX_OUT").Add(output_buffer_.release(), cc->InputTimestamp());
        AISDK_LOG_TRACE("[MonoBinoSwitchCalculator] Process complete");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
