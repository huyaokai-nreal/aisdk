#include "landmark_filter.h"

#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/base/profiling.h"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {

/**
 * @brief 初始化关键点滤波模型
 *
 * 该函数完成滤波模型的加载和基础配置，为后续时序关键点滤波处理做准备。
 * 主要包括基础网络初始化、输入输出张量获取和配置解析。
 *
 * @param[in] algo 网络算法配置（计算图优化策略等）
 * @param[in] model 模型结构配置（模型文件路径等）
 * @param[in] session 运行时会话参数（线程数/设备选择等）
 * @return absl::Status 初始化状态（OK表示成功）
 *
 * @note 初始化流程：
 *  1. 基类初始化：完成基础网络结构搭建
 *  2. 输入输出张量获取：获取模型输入输出层信息
 *  3. 调试支持：可选打印张量信息
 *  4. 布局配置：解析输入输出张量格式
 *  5. 参数预分配：准备归一化缩放系数
 */
absl::Status LandmarkFilter::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                  aisdk::xengine::SessionConfig &session) {
    // step1：调用基类初始化方法，完成公共网络参数设置
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }

    // step2: 获取input, output tensor信息
    itensor = m_net->GetInputTensors();
    otensor = m_net->GetOutputTensors();

    // step3: 输出调试信息
    if (aisdk::base::DebugProfiling::Get().GetOpt().aisdk_init_report) {
        PrintfHalIoTensors(itensor);
        PrintfHalIoTensors(otensor);
    }

    // step4: 解析input和output tensor的format信息
    itensor_format = checkshapeformat(model.vendor_type, itensor.m_tensors[0].m_rank);
    otensor_format = checkshapeformat(model.vendor_type, otensor.m_tensors[0].m_rank);

    // step5: 初始化归一化参数（42维缩放系数对应21个关键点，每个关键点有x,y两个维度）
    abs_scale.resize(42);
    return ret;
}

/**
 * @brief 关键点滤波的预处理操作
 *
 * 该函数实现输入关键点序列的归一化和标准化处理，为模型推理准备输入数据。主要完成：
 * 1. 输入有效性验证
 * 2. 中心化处理（以手掌中心为基准）
 * 3. 缩放系数计算
 * 4. 归一化坐标转换
 * 5. 张量数据填充
 *
 * @param[in] net_input 输入关键点序列（多帧，每帧21个关键点）
 *
 * @warning 假设输入关键点：
 *  - 每帧包含21个关键点（手部标准模型）
 *  - 索引9对应手掌根部关键点
 */
void LandmarkFilter::PreProcess(const std::vector<std::vector<Vec2f_t>> &net_input) {
    // step1：校验输入批量与模型配置的一致性
    int ai = itensor.m_batch * itensor.m_multishape_num;
    int bi = 1;
    if (ai != bi || itensor.m_packed_bybatch == false) {
        return;
    }

    int height = 0;
    int width = 0;
    int channels = 0;

    // step2: 遍历配置的输入张量（实际只处理第一个元素）
    for (int i = 0; i < bi; i++) {
        // step2.1 计算张量索引
        int multi_i = i / itensor.m_batch;  // 多输入张量索引（总是0）
        int batch_i = i % itensor.m_batch;  // 批次内索引（总是0）

        // step2.2 解析张量布局配置
        if (itensor_format == aisdk::xengine::TensorFormat::CHW) {
            channels = itensor.m_tensors[multi_i].m_dims[0];
            height = itensor.m_tensors[multi_i].m_dims[1];
            width = itensor.m_tensors[multi_i].m_dims[2];
        } else if (itensor_format == aisdk::xengine::TensorFormat::HWC) {
            height = itensor.m_tensors[multi_i].m_dims[0];
            width = itensor.m_tensors[multi_i].m_dims[1];
            channels = itensor.m_tensors[multi_i].m_dims[2];
        } else {
            // do nothing
        }

        // step2.3 获取输入数据
        int element_byte = itensor.m_tensors[multi_i].m_elementbyte;
        int mem_size = height * width * channels * element_byte;
        char *mem = (char *)itensor.m_tensors[multi_i].m_viraddr + batch_i * mem_size;
        float *temp = (float *)mem;

        // step3: 关键点归一化处理（获取当前序列的第一帧关键点，作为参考帧）
        std::vector<Vec2f_t> inputKptS = net_input[0];

        // step3.1 计算中心点（手掌根部关键点）
        m_center_uv = inputKptS[9];

        // step3.2 计算各个方向绝对偏移量
        for (int i = 0; i < inputKptS.size(); i++) {
            inputKptS[i] -= m_center_uv;                       // 相对中心点的偏移量
            abs_scale[i * 2 + 0] = std::abs(inputKptS[i][0]);  // 记录x方向的绝对值
            abs_scale[i * 2 + 1] = std::abs(inputKptS[i][1]);  // 记录y方向的绝对值
        }

        // step3.3 确定最大偏移量（所有方向中的最大值）
        m_scale = *std::max_element(abs_scale.begin(), abs_scale.end());

        // step3.4 遍历所有帧进行归一化
        for (int i = 0; i < net_input.size(); i++) {
            for (int p = 0; p < 21; p++) {
                /**
                 * X坐标归一化：(关键点X - 中心X) / 最大偏移
                 * Y坐标归一化：(关键点Y - 中心Y) / 最大偏移
                 */
                temp[i * 42 + p * 2] = (net_input[i][p][0] - m_center_uv[0]) / m_scale;
                temp[i * 42 + p * 2 + 1] = (net_input[i][p][1] - m_center_uv[1]) / m_scale;
            }
        }
    }
}

/**
 * @brief 关键点滤波的后处理操作
 *
 * @param[out] result 存储滤波后的关键点坐标（原始坐标系）
 *
 * @warning 基于预处理阶段的中心点和缩放因子进行逆变换
 */
void LandmarkFilter::PostProcess(std::vector<Vec2f_t> &result) {
    // step1: 输出格式验证
    if (otensor.m_packed_bybatch == false) {
        return;
    }

    int height = 0;
    int width = 0;
    int channel = 0;

    // step2: 遍历所有输出层，处理每个batch的数据
    for (int multi_i = 0; multi_i < otensor.m_multishape_num; multi_i++) {
        for (int batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
            if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                channel = otensor.m_tensors[multi_i].m_dims[0];
                height = otensor.m_tensors[multi_i].m_dims[1];
                width = otensor.m_tensors[multi_i].m_dims[2];
            } else if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                height = otensor.m_tensors[multi_i].m_dims[0];
                width = otensor.m_tensors[multi_i].m_dims[1];
                channel = otensor.m_tensors[multi_i].m_dims[2];
            } else {
                // do nothing
            }

            // step3: 获取具体输出数据
            int element_byte = otensor.m_tensors[multi_i].m_elementbyte;
            char *mem =
                (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * height * width * channel * element_byte;
            float *_data = (float *)mem;

            // step4: 反归一化坐标转换
            result.resize(21);
            for (int i = 0; i < 21; i++) {
                /**
                 * 公式：原始坐标 = 归一化坐标 × 缩放因子 + 中心点偏移
                 * X坐标反变换：_data[i * 2] * m_scale + m_center_uv[0]
                 * Y坐标反变换：_data[i * 2 + 1] * m_scale + m_center_uv[1]
                 */
                result[i] = {_data[i * 2] * m_scale + m_center_uv[0], _data[i * 2 + 1] * m_scale + m_center_uv[1]};
            }
        }
    }
}

/**
 * @brief 执行端到端的关键点时序滤波
 *
 * @param[in] baseinput 输入关键点序列（多帧数据）
 * @param[out] baseresult 输出滤波后的关键点（单帧结果）
 * @return absl::Status 网络推理状态（OK表示成功）
 *
 * @warning 返回状态仅反映网络运行情况，不包含预处理/后处理错误
 */
absl::Status LandmarkFilter::Inference(const std::vector<std::vector<Vec2f_t>> &baseinput,
                                       std::vector<Vec2f_t> &baseresult) {
    // step1: 预处理
    PreProcess(baseinput);

    // step2: 模型推理
    auto ret = m_net->RunNet();

    // step3: 后处理
    PostProcess(baseresult);
    return ret;
}

}  // namespace aisdk::algorithm
