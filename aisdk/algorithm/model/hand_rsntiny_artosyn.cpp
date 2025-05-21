#include "hand_rsntiny_artosyn.h"

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include <cstdint>

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
void ArtosynRSNTiny::ipr(float *__restrict input_hm, float *__restrict kpt_x_out, float *__restrict kpt_y_out) {
    // 在最后一个维度（空间维度）执行softmax，将热图转换为概率分布
    // 输入形状解释为 [1, 21, 32 * 32]，输出hm_softmax_保持相同形状
    softmax_last_dim(input_hm, m_hm_softmax_.data(), {1, m_keypoint_num_, m_output_shape_ * m_output_shape_});

    // 列方向压缩：对每个关键点的空间热图按行求和（维度h）
    // 输入：hm_softmax_[21,32,32] -> 输出：hm_reduce_col_[21,32]
    reduce_sum_h(m_hm_softmax_.data(), m_hm_reduce_col_.data(), m_keypoint_num_, m_output_shape_, m_output_shape_);

    // 行方向压缩：对每个关键点的空间热图按列求和（维度w）
    // 输入：hm_softmax_[21,32,32] -> 输出：hm_reduce_row_[21,32]
    reduce_sum_w(m_hm_softmax_.data(), m_hm_reduce_row_.data(), m_keypoint_num_, m_output_shape_, m_output_shape_);

    // 列分布修正：将列方向概率与预计算系数相乘（亚像素补偿）
    // m_hm_reduce_col_[21,32] × m_mul_coeff_[32] → m_hm_reduce_col_row_[21,32]
    elementwise_mult_hm(m_hm_reduce_col_.data(), m_mul_coeff_.data(), m_hm_reduce_col_row_.data(), m_keypoint_num_,
                        m_keypoint_num_ * m_output_shape_);

    // 行分布修正：将行方向概率与预计算系数相乘（亚像素补偿）
    // m_hm_reduce_row_[21,32] × m_mul_coeff_[32] → m_hm_reduce_row_col_[21,32]
    elementwise_mult_hm(m_hm_reduce_row_.data(), m_mul_coeff_.data(), m_hm_reduce_row_col_.data(), m_keypoint_num_,
                        m_keypoint_num_ * m_output_shape_);

    // 计算x坐标：对修正后的列分布沿宽度维度求和，得到期望坐标
    // 输入：hm_reduce_col_row_[21,32] → 输出：kpt_x_out[21]
    reduce_sum_w(m_hm_reduce_col_row_.data(), kpt_x_out, m_keypoint_num_, 1, m_output_shape_);

    // 计算y坐标：对修正后的行分布沿高度维度求和，得到期望坐标
    // 输入：hm_reduce_row_col_[21,32] → 输出：kpt_y_out[21]
    reduce_sum_h(m_hm_reduce_row_col_.data(), kpt_y_out, m_keypoint_num_, m_output_shape_, 1);
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
 * 2. 输入类型校验：强制要求输入数据为BLOB格式（二进制大对象）
 * 3. 输入尺寸获取：从模型输入层获取输入图像分辨率（128x128）
 * 4. 输出层解析：从名为"feat"的输出层获取热图维度信息
 * 5. 缓存预分配：根据输出维度初始化中间计算缓冲区
 * 6. 系数初始化：生成亚像素修正系数（mul_coeff_）
 *
 * @warning 模型必须包含名为"input"的输入层和"feat"的输出层
 */
absl::Status ArtosynRSNTiny::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                  aisdk::xengine::SessionConfig &session) {
    // step1：调用基类初始化方法，完成公共网络参数设置
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }

    // step2：验证输入类型为BLOB格式（二进制大对象）
    if (m_input_category != aisdk::xengine::ImageCategory::IS_BLOB) {
        return absl::InternalError("model input is not blob");
    }

    // step3：获取输入层索引并解析输入尺寸
    int index_input = m_net->GetInputImageBlobsIndex("input");       // 根据名称查找输入层
    m_input_shape_ = iImageblobs.m_imageblobs[index_input].m_width;  // 输入分辨率（128）

    // step4：解析输出层维度信息
    int feat_c = 0;
    int feat_h = 0;
    int feat_w = 0;
    int index_feat = m_net->GetOutputTensorIndex("feat");  // 根据名称查找输出层
    aisdk::xengine::ArtosynTensorDims &feat_dims = otensor.m_tensors[index_feat].m_artosyn_dims;
    feat_c = feat_dims.u32OriChannels;  // 关键点通道数（21）
    feat_h = feat_dims.u32Height;       // 热图高度（32）
    feat_w = feat_dims.u32Width;        // 热图宽度（32）

    // step5.1：预分配模型输出缓存（NCHW格式）
    m_outputsNCHW.resize(feat_c * feat_h * feat_w);  // 21x32x32=21504元素

    // step5.2：初始化后处理相关参数
    m_output_shape_ = feat_w;  // 热图分辨率32
    m_keypoint_num_ = feat_c;  // 关键点数量21

    // step5.3：预分配中间计算缓冲区
    m_hm_softmax_.resize(m_keypoint_num_ * m_output_shape_ * m_output_shape_);  // softmax后的热图 21x32x32
    m_hm_reduce_col_.resize(m_keypoint_num_ * m_output_shape_);                 // 列压缩缓存 21x32
    m_hm_reduce_row_.resize(m_keypoint_num_ * m_output_shape_);                 // 行压缩缓存 21x32
    m_hm_reduce_col_row_.resize(m_keypoint_num_ * m_output_shape_);             // 列修正缓存 21x32
    m_hm_reduce_row_col_.resize(m_keypoint_num_ * m_output_shape_);             // 行修正缓存 21x32

    // step6：生成亚像素修正系数（0.0~31.0的线性序列）
    m_mul_coeff_ = linspace<float>(0, 1, m_output_shape_, false);  // 生成32个等间距系数
    return ret;
}

/**
 * @brief 预处理输入图像数据，将其转换为模型所需的二进制格式
 * @param net_input 输入图像集合，要求为灰度图格式
 *
 * @note 处理流程：
 * 1. 输入有效性校验：检查批量大小和输入数据是否已预处理
 * 2. 遍历所有输入图像：
 *    - 验证图像格式为GRAY（单通道）
 *    - 获取模型输入缓冲区的内存布局参数
 *    - 执行内存拷贝，处理可能的宽度对齐需求
 *
 * @warning 特殊处理逻辑：
 * - 仅支持灰度图输入（GRAY格式），其他格式图像会被跳过
 * - 当输入图像宽度不等于模型要求的步长（width_s）时，自动填充右侧空白区域
 * - 内存拷贝使用虚拟地址直接操作，依赖硬件加速的内存管理单元（MMU）
 */
void ArtosynRSNTiny::PreProcess(const std::vector<Image> &net_input) {
    // step1：校验输入批量与模型配置的一致性
    int ai = iImageblobs.m_batch * iImageblobs.m_multiinput_num;  // 计算模型预期输入总数
    int bi = net_input.size();                                    // 实际输入数量
    if (ai != bi || iImageblobs.m_packed_bybatch == true) {
        return;
    }

    // step2：遍历处理每个输入图像
    int multi_i = 0;
    int batch_i = 0;
    int height = 0;
    int width = 0;
    int width_s = 0;
    int channels = 0;
    int element_byte = 0;
    for (int i = 0; i < bi; i++) {
        // 获取当前图像数据（OpenCV矩阵格式）
        auto &img = net_input[i].m_mat;

        // 初始化默认尺寸（128x128）
        int width = 128;
        int height = 128;

        // 解析输入索引（多输入多批次场景）
        multi_i = i / iImageblobs.m_batch;  // 多输入索引
        batch_i = i % iImageblobs.m_batch;  // 批次内索引
        auto index = i;                     // 实际使用的内存索引

        // step2.1：校验输入格式为GRAY
        if (iImageblobs.m_imageblobs[index].m_format == aisdk::xengine::ImageFormat::GRAY) {
            channels = 1;  // 灰度图单通道
        } else {
            continue;
        }

        // step2.2：获取内存布局参数
        height = iImageblobs.m_imageblobs[index].m_height;             // 图像实际高度
        width = iImageblobs.m_imageblobs[index].m_width;               // 图像实际宽度
        width_s = iImageblobs.m_imageblobs[index].m_wstride;           // 内存对齐后的步长
        element_byte = iImageblobs.m_imageblobs[index].m_elementbyte;  // 每个像素的字节数

        // step2.3：获取内存地址指针
        char *src_mem = (char *)img.data;                                      // 源数据地址（输入图像）
        char *dst_mem = (char *)iImageblobs.m_imageblobs[index].m_viraddr[0];  // 目标地址（模型输入缓冲区）

        // step2.4：执行内存拷贝（考虑内存对齐）
        if (width == width_s) {  // 当实际宽度等于步长时直接整块拷贝
            unsigned int mem_size = height * width * channels * element_byte;
            memcpy(dst_mem, src_mem, mem_size);
        } else {  // 需要逐行拷贝并填充对齐
            for (uint32_t hi = 0; hi < height; hi++) {
                // 拷贝有效数据部分
                memcpy(dst_mem, src_mem, width);

                // 填充对齐空白区域（置零）
                memset(dst_mem + width, 0, width_s - width);

                // 移动指针到下一行
                src_mem += width;
                dst_mem += width_s;
            }
        }
    }
}

/**
 * @brief 后处理函数，将模型输出解析为二维关键点坐标
 * @param result 输出参数，存储解析后的关键点坐标结果
 *
 * @note 处理流程：
 * 1. 获取输出层信息：深度图(depth)和热图特征(feat)的维度参数
 * 2. 初始化结果存储结构：根据批次大小预分配内存
 * 3. 遍历每个批次：
 *    a. 从NPU输出内存中提取热图数据
 *    b. 使用IPR方法计算亚像素级坐标
 *    c. 将热图坐标映射回输入图像分辨率
 */
void ArtosynRSNTiny::PostProcess(Kpt2dResult &result) {
    // 步骤1：获取输出层元数据
    int index_depth = m_net->GetOutputTensorIndex("depth");
    int index_feat = m_net->GetOutputTensorIndex("feat");

    // 解析输出层维度信息
    aisdk::xengine::ArtosynTensorDims &depth_dims = otensor.m_tensors[index_depth].m_artosyn_dims;
    aisdk::xengine::ArtosynTensorDims &feat_dims = otensor.m_tensors[index_feat].m_artosyn_dims;
    int depth_c = depth_dims.u32OriChannels;  // 深度图通道数
    int depth_h = depth_dims.u32Height;       // 深度图高度
    int depth_w = depth_dims.u32Width;        // 深度图宽度
    float *depth_data = (float *)otensor.m_tensors[index_depth].m_viraddr;

    int feat_c = feat_dims.u32OriChannels;  // 关键点通道数（21）
    int feat_h = feat_dims.u32Height;       // 热图高度（32）
    int feat_w = feat_dims.u32Width;        // 热图宽度（32）
    float *feat_data = (float *)otensor.m_tensors[index_feat].m_viraddr;

    // 步骤2：准备结果存储结构
    result.kpts.resize(otensor.m_batch);
    for (int batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
        auto &rsnkpt = result.kpts[batch_i];  // 当前批次的结果引用
        rsnkpt.resize(m_keypoint_num_);       // 预分配21个关键点存储

        // 步骤3：从NPU内存提取热图数据（格式转换）
        int feat_idx_group, feat_idx_score, feat_idx_left, feat_idx_right;
        std::vector<float> storedata(feat_c * feat_h * feat_w);

        // 三维遍历：通道->高度->宽度
        for (int idx_c = 0; idx_c < feat_c; idx_c++) {
            for (int idx_i = 0; idx_i < feat_h; idx_i++) {      // y轴
                for (int idx_j = 0; idx_j < feat_w; idx_j++) {  // x轴
                    // 计算内存索引（考虑批次/通道/空间布局）
                    feat_idx_group = ArtosynNpuGetEntryIndex(batch_i, otensor.m_batch, idx_i, idx_j, idx_c,
                                                             sizeof(float), feat_dims);
                    // 将数据重新排列为连续内存格式[C,H,W]
                    storedata[idx_c * feat_h * feat_w + idx_i * feat_w + idx_j] = feat_data[feat_idx_group];
                }
            }
        }

        // // 步骤4：亚像素坐标计算
        std::vector<float> kpt_x_data(m_keypoint_num_);               // X坐标缓存
        std::vector<float> kpt_y_data(m_keypoint_num_);               // Y坐标缓存
        ipr(storedata.data(), kpt_x_data.data(), kpt_y_data.data());  // 调用改进后处理

        // 步骤5：坐标映射到输入分辨率
        const float scale = static_cast<float>(m_input_shape_) / m_output_shape_;
        for (size_t i = 0; i < m_keypoint_num_; i++) {
            // 热图坐标(0-32) -> 输入图像坐标(0-128)
            rsnkpt[i][0] = kpt_x_data[i] * scale;  // X坐标映射
            rsnkpt[i][1] = kpt_y_data[i] * scale;  // Y坐标映射
        }
    }
}

/**
 * @brief 执行端到端的手部关键点检测推理流程
 * @param baseinput 输入图像集合，要求符合模型输入规格（如128x128灰度图）
 * @return absl::StatusOr<Kpt2dResult> 包含关键点检测结果或错误状态
 *
 * @note 完整推理流程：
 * 1. 预处理：将输入图像转换为模型需要的二进制格式（包括归一化、格式转换等）
 * 2. 网络推理：在NPU/CPU上执行神经网络前向计算
 * 3. 后处理：解析网络输出为可用的二维坐标结果
 */
absl::StatusOr<Kpt2dResult> ArtosynRSNTiny::Inference(const std::vector<Image> &baseinput) {
    PreProcess(baseinput);
    Kpt2dResult baseresult;
    absl::Status ret = m_net->RunNet();
    if (ret.ok()) {
        PostProcess(baseresult);
        return baseresult;
    }

    return absl::UnavailableError("failed to get 2d hand kpt result from ArtosynRSNTiny");
}

}  // namespace aisdk::algorithm
