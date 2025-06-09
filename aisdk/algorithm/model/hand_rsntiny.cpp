#include "hand_rsntiny.h"

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include "aisdk/algorithm/common/math.h"
#include "aisdk/algorithm/func/elementwise_mul.h"
#include "aisdk/algorithm/func/permute.h"
#include "aisdk/algorithm/func/reducesum.h"
#include "aisdk/algorithm/func/softmax.h"
#include "aisdk/base/log.h"
namespace aisdk::algorithm {

/**
 * @description:
 * @param {float* restrict} input_hm: input heatmap, 1x32x32x32
 * @param {float* restrict} kpt_x_out: output x coords, normalized to 0-1, 1x21
 * @param {float* restrict} kpt_y_out: output y coords, normalized to 0-1, 1x21
 * @return {*}
 */

/**
 * @brief 改进的热图坐标回归方法（Improved Post-Refinement, IPR）
 * @param input_hm 输入热图指针（形状为[m_keypoint_num_, height=32, width=32]）
 * @param kpt_x_out 输出x坐标数组（长度keypoint_num_）
 * @param kpt_y_out 输出y坐标数组（长度keypoint_num_）
 * @note 算法流程：
 *        1. Softmax归一化：对每个关键点的空间位置进行概率归一化
 *           - 输入热图形状解释为 [batch=1, m_keypoint_num_, height*width=1024]
 *           - 输出hm_softmax_形状保持与输入一致，数值表示空间概率分布
 *
 *        2. 行列方向概率压缩：
 *           - reduce_sum_h: 沿行方向压缩，得到列方向概率分布 m_hm_reduce_col_
 *             （形状[m_keypoint_num_, width=32]，每个元素表示该列在所有行的累积概率）
 *           - reduce_sum_w: 沿列方向压缩，得到行方向概率分布 m_hm_reduce_row_
 *             （形状[m_keypoint_num_, height=32]，每个元素表示该行在所有列的累积概率）
 *
 *        3. 坐标修正系数应用：
 *           - elementwise_mult_hm: 将行列概率分布与预计算系数mul_coeff_相乘
 *             - mul_coeff_包含位置偏移权重（如高斯加权系数或亚像素补偿系数）
 *             - m_hm_reduce_col_row_: 列分布修正后数据（用于最终x坐标计算）
 *             - m_hm_reduce_row_col_: 行分布修正后数据（用于最终y坐标计算）
 *
 *        4. 期望值计算：
 *           - reduce_sum_w: 对修正后的列分布沿宽度求和，得到x坐标期望值
 *             （kpt_x_out[m_keypoint_num_] 每个元素对应关键点的x坐标）
 *           - reduce_sum_h: 对修正后的行分布沿高度求和，得到y坐标期望值
 *             （kpt_y_out[m_keypoint_num_] 每个元素对应关键点的y坐标）
 *
 * @note 方法优势：
 *        - 相比直接取argmax坐标，通过概率分布的期望计算可获得亚像素级精度
 *        - 使用mul_coeff_系数补偿热图下采样带来的量化误差
 *        - 行列分离计算减少内存访问冲突，提高并行效率
 */
void RSNTiny::ipr(float *__restrict input_hm, float *__restrict kpt_x_out, float *__restrict kpt_y_out) {
    // 在最后一个维度（空间维度）执行softmax，将热图转换为概率分布
    // 输入形状解释为 [1, 21, 32 * 32]，输出hm_softmax_保持相同形状
    softmax_last_dim(input_hm, hm_softmax_.data(), {1, keypoint_num_, output_shape_ * output_shape_});

    // 列方向压缩：对每个关键点的空间热图按行求和（维度h）
    // 输入：hm_softmax_[21,32,32] -> 输出：hm_reduce_col_[21,32]
    reduce_sum_h(hm_softmax_.data(), hm_reduce_col_.data(), keypoint_num_, output_shape_, output_shape_);

    // 行方向压缩：对每个关键点的空间热图按列求和（维度w）
    // 输入：hm_softmax_[21,32,32] -> 输出：hm_reduce_row_[21,32]
    reduce_sum_w(hm_softmax_.data(), hm_reduce_row_.data(), keypoint_num_, output_shape_, output_shape_);

    // 列分布修正：将列方向概率与预计算系数相乘（亚像素补偿）
    // m_hm_reduce_col_[21,32] × m_mul_coeff_[32] → m_hm_reduce_col_row_[21,32]
    elementwise_mult_hm(hm_reduce_col_.data(), mul_coeff_.data(), hm_reduce_col_row_.data(), keypoint_num_,
                        keypoint_num_ * output_shape_);

    // 行分布修正：将行方向概率与预计算系数相乘（亚像素补偿）
    // m_hm_reduce_row_[21,32] × m_mul_coeff_[32] → m_hm_reduce_row_col_[21,32]
    elementwise_mult_hm(hm_reduce_row_.data(), mul_coeff_.data(), hm_reduce_row_col_.data(), keypoint_num_,
                        keypoint_num_ * output_shape_);

    // 计算x坐标：对修正后的列分布沿宽度维度求和，得到期望坐标
    // 输入：hm_reduce_col_row_[21,32] → 输出：kpt_x_out[21]
    reduce_sum_w(hm_reduce_col_row_.data(), kpt_x_out, keypoint_num_, 1, output_shape_);

    // 计算y坐标：对修正后的行分布沿高度维度求和，得到期望坐标
    // 输入：hm_reduce_row_col_[21,32] → 输出：kpt_y_out[21]
    reduce_sum_h(hm_reduce_row_col_.data(), kpt_y_out, keypoint_num_, output_shape_, 1);
}

/**
 * @brief 初始化手部关键点检测网络模型
 * @param algo 网络算法配置（计算图结构、内存分配策略等）
 * @param model 模型配置（模型文件路径、输入输出设置等）
 * @param session 会话配置（线程数、执行设备等）
 * @return absl::Status 初始化状态，OK表示成功，否则包含错误信息
 *
 * @note 初始化流程：
 * 1. 基类初始化：完成公共网络参数加载和基础结构准备
 * 2. 输入尺寸获取：从模型输入层获取输入图像分辨率（128x128）
 * 3. 输出层解析：从名为"feat"的输出层获取热图维度信息
 * 4. 缓存预分配：根据输出维度初始化中间计算缓冲区
 * 5. 系数初始化：生成亚像素修正系数（mul_coeff_）
 *
 * @warning 模型必须包含名为"input"的输入层和"feat"的输出层
 */
absl::Status RSNTiny::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                           aisdk::xengine::SessionConfig &session) {
    // step1：调用基类初始化方法，完成公共网络参数设置
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }

    // step2：配置输出输出张量参数
    itensor_format_ = itensor.m_tensors[0].m_dimtype;
    otensor_format_ = otensor.m_tensors[0].m_dimtype;

    // 预分配NCHW格式输出缓冲区（适配不同硬件后端）
    m_outputsNCHW.resize(otensor.m_tensors[0].m_elementsize);

    // step3: 解析模型维度参数
    input_shape_ = itensor.m_tensors[0].m_dims[1];   // 输入空间尺寸（默认正方形）
    output_shape_ = otensor.m_tensors[0].m_dims[1];  // 输出热图空间尺寸
    keypoint_num_ = otensor.m_tensors[0].m_dims[2];  // 手部关键点数量

    // step4: 预分配后处理缓冲区（根据输出热图尺寸和关键点数量规格化内存）
    hm_softmax_.resize(keypoint_num_ * output_shape_ * output_shape_);  // 单关键点热图softmax缓存
    hm_reduce_col_.resize(keypoint_num_ * output_shape_);               // 列方向概率聚合缓存
    hm_reduce_row_.resize(keypoint_num_ * output_shape_);               // 行方向概率聚合缓存
    hm_reduce_col_row_.resize(keypoint_num_ * output_shape_);           // 列-行双阶段聚合缓存
    hm_reduce_row_col_.resize(keypoint_num_ * output_shape_);           // 行-列双阶段聚合缓存

    // step5: 生成ipr（反向比例回归）系数
    mul_coeff_ = linspace<float>(0, 1, output_shape_, false);
    return ret;
}

/**
 * @brief 执行图像预处理操作
 *
 * 该函数实现输入图像的格式转换和内存拷贝，将原始图像数据适配到模型输入张量格式。
 * 主要处理包括：尺寸验证、布局调整和数据类型转换。
 *
 * @param[in] net_input 输入图像集合（OpenCV矩阵格式）
 *
 * @warning 前置条件：
 *  - 输入图像尺寸须符合张量配置要求
 *  - 图像通道数必须匹配模型输入通道数
 */
void RSNTiny::PreProcess(const std::vector<Image> &net_input) {
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
        int element_byte = itensor.m_tensors[multi_i].m_elementbyte;  //元素字节数
        unsigned int mem_size = height * width * channels * element_byte;
        char *mem = (char *)itensor.m_tensors[multi_i].m_viraddr + batch_i * mem_size;

        // step5: 处理图像（将输入图像转换为单精度浮点类型（CV_32FC1））
        cv::Mat image_resized;
        img.convertTo(image_resized, CV_32FC1);
        memcpy(mem, image_resized.data, mem_size);
    }
}

/**
 * @brief 执行模型输出后处理操作（模型只会输出feat）
 *
 * 该函数将原始输出热力图解析为2D关键点坐标，主要完成以下任务：
 * 1. 输出张量格式验证
 * 2. 热力图数据内存布局转换
 * 3. 核心坐标解析算法(IPR)执行
 * 4. 热图坐标到原始图像坐标的映射
 *
 * @param[out] result 存储处理后的关键点检测结果
 */
void RSNTiny::PostProcess(Kpt2dResult &result) {
    // step1: 输出格式验证
    if (!otensor.m_packed_bybatch) {
        return;
    }

    AISDK_LOG_TRACE("rsntiny, output size {}, batch {}", otensor.m_multishape_num, otensor.m_batch);

    // step2: 结果内存预分配
    result.kpts.resize(otensor.m_batch);
    int height = 0;
    int width = 0;
    int channel = 0;

    // step3: 遍历处理output tensor的内容
    for (size_t multi_i = 0; multi_i < otensor.m_multishape_num; multi_i++) {
        for (size_t batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
            // step3.1 解析otensor的channel, height, width
            if (otensor_format_ == aisdk::xengine::TensorFormat::CHW) {
                channel = otensor.m_tensors[multi_i].m_dims[0];
                height = otensor.m_tensors[multi_i].m_dims[1];
                width = otensor.m_tensors[multi_i].m_dims[2];
            } else if (otensor_format_ == aisdk::xengine::TensorFormat::HWC) {
                height = otensor.m_tensors[multi_i].m_dims[0];
                width = otensor.m_tensors[multi_i].m_dims[1];
                channel = otensor.m_tensors[multi_i].m_dims[2];
            } else {
                // do nothing
            }

            // step3.2 获取输出具体数据信息
            int element_byte = otensor.m_tensors[multi_i].m_elementbyte;
            char *mem =
                (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * height * width * channel * element_byte;
            float *_data = (float *)mem;

            auto &rsnkpt = result.kpts[batch_i];
            if (rsnkpt.size() != keypoint_num_) {
                rsnkpt.resize(keypoint_num_);
            }

            // step3.3 对输出结果进行IPR处理（单输出 "feat"）
            std::vector<float> kpt_x_data(keypoint_num_);
            std::vector<float> kpt_y_data(keypoint_num_);
            if (otensor_format_ == aisdk::xengine::TensorFormat::CHW) {
                ipr(_data, kpt_x_data.data(), kpt_y_data.data());
            } else if (otensor_format_ == aisdk::xengine::TensorFormat::HWC) {
                NHWC2NCHW(_data, m_outputsNCHW.data(), 1, channel, height * width);
                ipr(m_outputsNCHW.data(), kpt_x_data.data(), kpt_y_data.data());
            } else {
                // do nothing
            }

            // step3.4 坐标映射（热图坐标 -> 原始坐标）
            for (size_t i = 0; i < keypoint_num_; i++) {
                rsnkpt[i][0] = kpt_x_data[i] * static_cast<float>(input_shape_);
                rsnkpt[i][1] = kpt_y_data[i] * static_cast<float>(input_shape_);
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
absl::StatusOr<Kpt2dResult> RSNTiny::Inference(const std::vector<Image> &baseinput) {
    // step1: 预处理
    PreProcess(baseinput);

    // step2: 模型推理
    Kpt2dResult baseresult;
    absl::Status ret = m_net->RunNet();
    if (ret.ok()) {
        // step3: 后处理
        PostProcess(baseresult);
        return baseresult;
    }

    return absl::UnavailableError("failed to get 2d hand kpt result from rsntiny");
}

}  // namespace aisdk::algorithm
