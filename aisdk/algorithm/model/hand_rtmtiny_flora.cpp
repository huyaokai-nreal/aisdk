#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include <numeric>

#include "aisdk/algorithm/common/math.h"
#include "aisdk/algorithm/func/softmax.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "hand_rtmtiny.h"
namespace aisdk::algorithm {

/**
 * @description:
 * @param {float* restrict} input_hm: input heatmap, 1x32x32x32
 * @param {float* restrict} kpt_x_out: output x coords, normalized to 0-1, 1x21
 * @param {float* restrict} kpt_y_out: output y coords, normalized to 0-1, 1x21
 * @return {*}
 */

/**
 * @brief 初始化RTM手部关键点检测模型
 *
 * 该函数完成模型基础配置解析和关键参数初始化，为模型推理做准备。
 * 继承自基础网络类，专注于RTM轻量级手部关键点模型的特殊配置。
 *
 * @param[in] algo 网络算法配置（计算图优化策略等）
 * @param[in] model 模型结构配置（输入输出层规格）
 * @param[in] session 运行时会话参数（硬件设备/线程池等）
 * @return absl::Status 初始化状态，OK表示成功，否则包含错误信息
 *
 * @warning 输出张量维度顺序：
 *  假设首层输出张量维度为 [keypoint_num, height, width]
 */
absl::Status RTMTinyFlora::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session) {
    AISDK_LOG_TRACE("aaaaaaaaaaa");
    // step1：调用基类初始化方法，完成公共网络参数设置
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }

    // step2: 配置input, output tensor信息
    itensor_format_ = itensor.m_tensors[0].m_dimtype;
    otensor_format_ = otensor.m_tensors[0].m_dimtype;

    // step3: 解析模型维度参数
    input_shape_ = itensor.m_tensors[0].m_dims[1];   // 输入图像尺寸（正方形）
    output_shape_ = otensor.m_tensors[0].m_dims[1];  // 输出热图尺寸（正方形）
    keypoint_num_ = otensor.m_tensors[0].m_dims[0];  // 输出手部关键点个数

    // step4: 初始化坐标变换系数（创建线性映射序列： 热图坐标 -> 原始图像坐标）
    mul_coeff_ = linspace<float>(0, 1, output_shape_, false);
    return ret;
}

/**
 * @brief 执行RTM模型的输入图像预处理
 *
 * 该函数实现输入图像到模型张量的格式转换和内存填充，主要处理包括：
 * 1. 输入图像数量校验
 * 2. 张量内存布局解析
 * 3. 图像数据转换和直接内存填充
 *
 * @param[in] net_input 输入图像集合（OpenCV矩阵格式）
 *
 * @warning 关键特性：
 *  - 通过cv::Mat视图实现零拷贝内存写入
 *  - 要求输入图像尺寸完全匹配张量配置
 */
void RTMTinyFlora::PreProcess(const std::vector<Image> &net_input) {
    // step1：校验输入批量与模型配置的一致性
    auto ai = itensor.m_batch * itensor.m_multishape_num;  // 预期输入
    auto bi = net_input.size();                            // 实际输入
    if (ai != bi || !itensor.m_packed_bybatch) {
        return;
    }

    int height = 0;
    int width = 0;
    int channels = 0;

    // step2: 遍历处理每张图片
    for (size_t i = 0; i < bi; i++) {
        auto &img = net_input[i].m_mat;

        // step2.1 计算多维张量索引（支持单批次多输入或多批次配置）
        int multi_i = i / itensor.m_batch;  // 多输入张量索引
        int batch_i = i % itensor.m_batch;  // 批次内索引

        // step3: 根据配置获取图像高度，宽度，通道数等信息
        if (itensor_format_ == aisdk::xengine::TensorFormat::CHW) {
            channels = itensor.m_tensors[multi_i].m_dims[0];
            height = itensor.m_tensors[multi_i].m_dims[1];
            width = itensor.m_tensors[multi_i].m_dims[2];
        } else if (itensor_format_ == aisdk::xengine::TensorFormat::HWC) {
            height = itensor.m_tensors[multi_i].m_dims[0];
            width = itensor.m_tensors[multi_i].m_dims[1];
            channels = itensor.m_tensors[multi_i].m_dims[2];
        } else {
            // do nothing
        }

        // step4: 获取图像信息
        int element_byte = itensor.m_tensors[multi_i].m_elementbyte;  // 元素字节数
        int mem_size = height * width * channels * element_byte;
        char *mem = (char *)itensor.m_tensors[multi_i].m_viraddr + batch_i * mem_size;

        // step5: 处理图像（将输入图像转换为单精度浮点类型（CV_32FC1））
        cv::Mat image_resized(img.size(), CV_32FC1, mem);
        img.convertTo(image_resized, CV_32FC1);
        image_resized = (image_resized - img_mean_) / img_std_;
    }
}

/**
 * @brief 执行模型输出后处理操作(模型输出feat_x, feat_y, feat_z)
 *
 * 该函数将原始输出热力图解析为2D关键点坐标，主要完成以下任务：
 * 1. 输出张量格式验证
 * 2. 热力图数据内存布局转换
 * 3. 核心坐标解析算法(IPR)执行
 * 4. 热图坐标到原始图像坐标的映射
 *
 * @param[out] result 存储处理后的关键点检测结果
 */
void RTMTinyFlora::PostProcess(Kpt2dResult &result) {
    // step1: 输出格式验证
    if (!otensor.m_packed_bybatch) {
        return;
    }

    // step2: 结果内存预分配
    result.kpts.resize(otensor.m_batch);
    result.rdepths.resize(otensor.m_batch);
    result.hold_labels.resize(otensor.m_batch);
    result.raw_feats.resize(otensor.m_batch);

    unsigned int element_byte;
    // step3: 遍历处理output tensor的内容
    for (size_t multi_i = 0; multi_i < otensor.m_multishape_num; multi_i++) {
        for (size_t batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
            result.hold_labels[batch_i] = false;
            if (otensor.m_tensors[multi_i].m_name == "held_cls") {
                element_byte = otensor.m_tensors[multi_i].m_elementbyte;
                char *mem = (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * element_byte;
                float *_data = (float *)mem;
                result.hold_labels[batch_i] = false;
                AISDK_LOG_TRACE(
                    fmt::format("[RTMTinyInferenceCalculator], hold_cls_score {:.4f}", static_cast<float>(_data[0])));
                if (_data[0] > hand_label_cls_thr) {
                    result.hold_labels[batch_i] = true;
                }
                continue;
            }
            if (otensor.m_tensors[multi_i].m_name == "raw_feats") {
                element_byte = otensor.m_tensors[multi_i].m_elementbyte;
                auto &raw_feats = result.raw_feats[batch_i];
                int channel = otensor.m_tensors[multi_i].m_dims[0];
                int width = otensor.m_tensors[multi_i].m_dims[1];
                int height = otensor.m_tensors[multi_i].m_dims[2];
                int mem_size = channel * height * width;
                char *mem =
                    (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * height * width * channel * element_byte;
                float *_data = (float *)mem;
                raw_feats.assign(_data, _data + mem_size);
                continue;
            }
            int channel = 1;
            int height = otensor.m_tensors[multi_i].m_dims[0];
            int width = otensor.m_tensors[multi_i].m_dims[1];
            element_byte = otensor.m_tensors[multi_i].m_elementbyte;

            // step3.2 获取输出具体数据信息
            char *mem =
                (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * height * width * channel * element_byte;
            float *_data = (float *)mem;
            auto &rsnkpt = result.kpts[batch_i];
            auto &rdepth = result.rdepths[batch_i];
            if (rsnkpt.size() != keypoint_num_) {
                rsnkpt.resize(keypoint_num_);
                rdepth.resize(keypoint_num_);
            }

            // step3.3 Softmax概率转换（将原始输出转换为概率分布）
            std::vector<float> kpt_softmax_data(channel * height * width);

            // 在空间维度应用softmax（width方向）
            softmax_last_dim(_data, kpt_softmax_data.data(), {1, height, width});

            // step4：处理不同输出层数据
            for (size_t i = 0; i < keypoint_num_; i++) {
                // step4.1: 处理x坐标输出层
                if (otensor.m_tensors[multi_i].m_name == "feat_x") {
                    rsnkpt[i][0] = std::inner_product(mul_coeff_.begin(), mul_coeff_.end(),
                                                      kpt_softmax_data.begin() + i * width, 0.0F) *
                                   static_cast<float>(input_shape_);
                }

                // step4.2: 处理y坐标输出层
                if (otensor.m_tensors[multi_i].m_name == "feat_y") {
                    rsnkpt[i][1] = std::inner_product(mul_coeff_.begin(), mul_coeff_.end(),
                                                      kpt_softmax_data.begin() + i * width, 0.0F) *
                                   static_cast<float>(input_shape_);
                }
                // step4.3: 处理深度输出层
                if (otensor.m_tensors[multi_i].m_name == "feat_z") {
                    rdepth[i] = (std::inner_product(mul_coeff_.begin(), mul_coeff_.end(),
                                                    kpt_softmax_data.begin() + i * width, 0.0F) -
                                 0.5) *
                                0.4;
                }
            }
        }
    }
}

/**
 * @brief 执行端到端手部关键点检测推理
 *
 * 该函数实现完整推理流水线：
 * 1. 输入图像预处理
 * 2. 网络前向计算
 * 3. 输出解析后处理
 *
 * @param[in] baseinput 输入图像集合（OpenCV格式图像）
 * @return absl::StatusOr<Kpt2dResult> 包含关键点结果或错误状态
 *
 * @warning 确保输入图像尺寸与模型输入要求一致
 */
absl::StatusOr<Kpt2dResult> RTMTinyFlora::Inference(const std::vector<Image> &baseinput) {
    // step1: 预处理
    {
        // TIMER_ONCE_WITH_TAG(RTMTinyFlora::Preprocess);
        PreProcess(baseinput);
    }

    // step2: 模型推理
    Kpt2dResult baseresult;
    absl::Status ret;
    {
        // TIMER_ONCE_WITH_TAG(RTMTinyFlora::RunNet);
        ret = m_net->RunNet();
    }

    // step3: 后处理
    if (ret.ok()) {
        {
            // TIMER_ONCE_WITH_TAG(RTMTinyFlora::PoseProcess);
            PostProcess(baseresult);
        }
        return baseresult;
    }

    return absl::UnavailableError("failed to get 2d hand kpt result from rtmtiny");
}

}  // namespace aisdk::algorithm
