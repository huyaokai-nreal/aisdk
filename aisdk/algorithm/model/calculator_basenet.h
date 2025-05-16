#pragma once
#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"
#include "aisdk/xengine/nr_model_mgr.h"
#include "aisdk/xengine/nrhal_net.h"
#include "aisdk/base/camera_model.h"

namespace aisdk::algorithm {
using BaseNetAlgoPtr = std::unique_ptr<aisdk::xengine::BaseNetAlgo,std::function<void(aisdk::xengine::BaseNetAlgo*)>>;

/**
 * @class CalculatorBaseNet
 * @brief calculator部分多个算子相关类的公共基类 - 负责网络算法实例的生命周期管理及输入/输出数据桥接
 * 
 * @details
 * 本类作为网络计算框架的核心基类，承担以下职责：
 * 1. 持有具体网络算法实例（BaseNetAlgo）的所有权
 * 2. 初始化网络算法实例并配置运行时参数
 * 3. 提供标准化的输入/输出数据接口
 * 4. 管理网络算法与框架其他组件的交互
 * 
 * @note
 * - 使用本类前必须通过 SetBaseNetAlgo() 设置有效的算法实例
 * - 初始化完成后可通过公共成员变量访问输入/输出张量描述符
 * - 本类不直接处理计算逻辑，需通过派生类实现具体运算行为
 */
class CalculatorBaseNet {
public:
    // 构造和析构函数
    CalculatorBaseNet() {}
    virtual ~CalculatorBaseNet() {}

    /**
     * @brief 设置基础网络算法实例（所有权转移）
     * @param[in] net 网络算法智能指针
     * @note
     * - 调用后本类持有算法实例的独占所有权
     * - 必须在调用 Init() 前设置有效实例
     */
    void SetBaseNetAlgo(BaseNetAlgoPtr& net) { m_net = std::move(net); }

    /**
     * @brief 初始化网络算法实例
     * @param[in] algo 网络算法配置参数
     * @param[in] model 模型加载配置
     * @param[in] session 运行时会话配置
     * @return absl::Status 初始化状态
     * @retval absl::OkStatus() 初始化成功
     * @retval absl::InternalError 算法实例未设置或初始化失败
     */
    absl::Status Init(aisdk::xengine::NetAlgoConfig& algo, aisdk::xengine::ModelConfig& model,
                                aisdk::xengine::SessionConfig& session) {
        if (m_net) {
            // 统一以json格式输入到算子中，各算子差异化解析
            if (algo.has_param) {
                m_net->SetAlgoParams(std::string("algo_param_json"), algo.algo_param);
            }

            // 初始化
            absl::Status ret;
            ret = m_net->Init(algo.net_unique_id, model, session);
            if (!ret.ok()) {
                AISDK_LOG_ERROR("calculator base net init failed");
                return ret;
            }
            
            // 获取基础图像信息
            m_input_category = m_net->GetInputImageCategory();
            iImageblobs = m_net->GetInputImageBlobs();

            // 获取模型输入输出张量
            itensor = m_net->GetInputTensors();
            AISDK_LOG_TRACE("itensor.m_tensors size: {}", itensor.m_tensors.size());
            otensor = m_net->GetOutputTensors();
            AISDK_LOG_TRACE("otensor.m_tensors size: {}", otensor.m_tensors.size());

            // 调试模式下，打印张量信息
            if (aisdk::base::DebugProfiling::Get().GetOpt().aisdk_init_report) {
                PrintfHalIoTensors(itensor);
                PrintfHalIoTensors(otensor);
            }

            return absl::OkStatus();
        }

        return absl::InternalError("failure");
    }

public:
    BaseNetAlgoPtr m_net = nullptr;                  // 实例
    aisdk::xengine::ImageCategory m_input_category;  // 输入图像类别（RGB/YUV等）
    aisdk::xengine::IoImageBlobs iImageblobs;        // 输入图像块描述符（分辨率/格式）
    aisdk::xengine::IoTensors itensor;               // 输入张量元数据描述（维度/数据类型等）
    aisdk::xengine::IoTensors otensor;               // 输出张量元输出描述
};

/**
 * @class DetectBaseNet
 * @brief 检测模型相关的calculator的抽象基类 - 定义检测任务的标准推理接口
 * @extends CalculatorBaseNet
 */
class DetectBaseNet : public CalculatorBaseNet {
public:
    virtual absl::Status Inference(const std::vector<Image> &baseinput, DetOutputInternal &baseresult) = 0;
};

/**
 * @class HandLandmarkBaseNet
 * @brief 2d模型相关的calculator的抽象基类 - 定义2d关键点检测任务的标准推理接口
 * @extends CalculatorBaseNet
 */
class HandLandmarkBaseNet: public CalculatorBaseNet {
    public:
    virtual absl::StatusOr<Kpt2dResult> Inference(const std::vector<Image> &input) = 0;

};

class LiftBaseNet: public CalculatorBaseNet {
    public:
    virtual absl::StatusOr<LiftNetOutputs> Inference(const LiftNetInputs& input) = 0;
    virtual absl::Status SetCameraInfo(const std::shared_ptr<base::BaseCameraModel>& left_camera, const std::shared_ptr<base::BaseCameraModel>& right_camera) = 0;
};

}  // namespace aisdk::algorithm
