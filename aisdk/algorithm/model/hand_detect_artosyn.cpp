#include "hand_detect_artosyn.h"

#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/base/profiling.h"
#include "aisdk/xengine/cv/xr_cv.h"
#include "aisdk/xengine/nrhal_common.h"
#include "aisdk/xengine/nrhal_define.h"

namespace aisdk::algorithm {

// 调用获取接口会崩溃，暂时这么写
void ArtosynHandDetectNetv2::ArtosynHandDetectNetV2reset() {
    itensor.m_tensors[0].m_name = "images";
    // itensor.m_tensors[0].m_dimtype = TensorFormat::NCHW;
    itensor.m_tensors[0].m_elementype = aisdk::xengine::ElementType::UINT8;
    itensor.m_tensors[0].m_elementbyte = 1;
    itensor.m_tensors[0].m_artosyn_dims.achName = "images";
    itensor.m_tensors[0].m_artosyn_dims.achType = "uint8";
    itensor.m_tensors[0].m_artosyn_dims.achStepType = "";
    itensor.m_tensors[0].m_artosyn_dims.achLayoutType = "";
    // itensor.m_tensors[0].m_artosyn_dims.achMemoryType = std::string(stTensor.achMemoryType);
    // itensor.m_tensors[0].m_artosyn_dims.achDdrFormat = std::string(stTensor.achDdrFormat);
    itensor.m_tensors[0].m_artosyn_dims.dScaleFactor = 0.000000;
    itensor.m_tensors[0].m_artosyn_dims.u32ID = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32Bank = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32Offset = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32Height = 192;
    itensor.m_tensors[0].m_artosyn_dims.u32KStep = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32KNormNum = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32KSizeLast = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32KSizeNorm = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32BitWidth = 8;
    itensor.m_tensors[0].m_artosyn_dims.u32Num = 1;
    itensor.m_tensors[0].m_artosyn_dims.u32OriChannels = 1;
    itensor.m_tensors[0].m_artosyn_dims.u32OriFrameSize = 1;
    itensor.m_tensors[0].m_artosyn_dims.u32Precision = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32RowStep = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32TensorStep = 0;
    itensor.m_tensors[0].m_artosyn_dims.u32Size = 147456;
    itensor.m_tensors[0].m_artosyn_dims.u32MemorySize = 147456;
    itensor.m_tensors[0].m_artosyn_dims.u32Width = 256;
    itensor.m_tensors[0].m_artosyn_dims.s32ZeroPoint = 0;

    otensor.m_tensors[0].m_name = "output_box";
    // otensor.m_tensors[0].m_dimtype = TensorFormat::NCHW;
    otensor.m_tensors[0].m_elementype = aisdk::xengine::ElementType::FLOAT32;
    otensor.m_tensors[0].m_elementbyte = 4;
    otensor.m_tensors[0].m_artosyn_dims.achName = "output_box";
    otensor.m_tensors[0].m_artosyn_dims.achType = "float";
    otensor.m_tensors[0].m_artosyn_dims.achStepType = "normal";
    otensor.m_tensors[0].m_artosyn_dims.achLayoutType = "float";
    // otensor.m_tensors[0].m_artosyn_dims.achMemoryType = std::string(stTensor.achMemoryType);
    // otensor.m_tensors[0].m_artosyn_dims.achDdrFormat = std::string(stTensor.achDdrFormat);
    otensor.m_tensors[0].m_artosyn_dims.dScaleFactor = 0.003891;
    otensor.m_tensors[0].m_artosyn_dims.u32ID = 74;
    otensor.m_tensors[0].m_artosyn_dims.u32Bank = 0;
    otensor.m_tensors[0].m_artosyn_dims.u32Offset = 4096;
    otensor.m_tensors[0].m_artosyn_dims.u32Height = 12;
    otensor.m_tensors[0].m_artosyn_dims.u32KStep = 768;
    otensor.m_tensors[0].m_artosyn_dims.u32KNormNum = 3;
    otensor.m_tensors[0].m_artosyn_dims.u32KSizeLast = 1;
    otensor.m_tensors[0].m_artosyn_dims.u32KSizeNorm = 1;
    otensor.m_tensors[0].m_artosyn_dims.u32BitWidth = 0;
    otensor.m_tensors[0].m_artosyn_dims.u32Num = 1;
    otensor.m_tensors[0].m_artosyn_dims.u32OriChannels = 4;
    otensor.m_tensors[0].m_artosyn_dims.u32OriFrameSize = 1;
    otensor.m_tensors[0].m_artosyn_dims.u32Precision = 32;
    otensor.m_tensors[0].m_artosyn_dims.u32RowStep = 64;
    otensor.m_tensors[0].m_artosyn_dims.u32TensorStep = 768;
    otensor.m_tensors[0].m_artosyn_dims.u32Size = 768;
    otensor.m_tensors[0].m_artosyn_dims.u32MemorySize = 3072;
    otensor.m_tensors[0].m_artosyn_dims.u32Width = 16;
    otensor.m_tensors[0].m_artosyn_dims.s32ZeroPoint = -128;

    otensor.m_tensors[1].m_name = "output_cls";
    // otensor.m_tensors[1].m_dimtype = TensorFormat::NCHW;
    otensor.m_tensors[1].m_elementype = aisdk::xengine::ElementType::FLOAT32;
    otensor.m_tensors[1].m_elementbyte = 4;
    otensor.m_tensors[1].m_artosyn_dims.achName = "output_cls";
    otensor.m_tensors[1].m_artosyn_dims.achType = "float";
    otensor.m_tensors[1].m_artosyn_dims.achStepType = "normal";
    otensor.m_tensors[1].m_artosyn_dims.achLayoutType = "float";
    // otensor.m_tensors[1].m_artosyn_dims.achMemoryType = std::string(stTensor.achMemoryType);
    // otensor.m_tensors[1].m_artosyn_dims.achDdrFormat = std::string(stTensor.achDdrFormat);
    otensor.m_tensors[1].m_artosyn_dims.dScaleFactor = 0.003920;
    otensor.m_tensors[1].m_artosyn_dims.u32ID = 76;
    otensor.m_tensors[1].m_artosyn_dims.u32Bank = 0;
    otensor.m_tensors[1].m_artosyn_dims.u32Offset = 0;
    otensor.m_tensors[1].m_artosyn_dims.u32Height = 12;
    otensor.m_tensors[1].m_artosyn_dims.u32KStep = 768;
    otensor.m_tensors[1].m_artosyn_dims.u32KNormNum = 3;
    otensor.m_tensors[1].m_artosyn_dims.u32KSizeLast = 1;
    otensor.m_tensors[1].m_artosyn_dims.u32KSizeNorm = 1;
    otensor.m_tensors[1].m_artosyn_dims.u32BitWidth = 0;
    otensor.m_tensors[1].m_artosyn_dims.u32Num = 1;
    otensor.m_tensors[1].m_artosyn_dims.u32OriChannels = 3;
    otensor.m_tensors[1].m_artosyn_dims.u32OriFrameSize = 1;
    otensor.m_tensors[1].m_artosyn_dims.u32Precision = 32;
    otensor.m_tensors[1].m_artosyn_dims.u32RowStep = 64;
    otensor.m_tensors[1].m_artosyn_dims.u32TensorStep = 768;
    otensor.m_tensors[1].m_artosyn_dims.u32Size = 768;
    otensor.m_tensors[1].m_artosyn_dims.u32MemorySize = 3072;
    otensor.m_tensors[1].m_artosyn_dims.u32Width = 16;
    otensor.m_tensors[1].m_artosyn_dims.s32ZeroPoint = -128;
    return;
}

bool ArtosynHandDetectNetv2::ChangeCHW2HWC(float *src, std::vector<float> &dest, int channels, int height, int width) {
    // 验证输入参数有效性
    int total_elements = channels * height * width;
    if ((!src) || (channels <= 0) || (height <= 0) || (width <= 0) || (dest.size() != total_elements)) {
        AISDK_LOG_ERROR(
            "[ArtosynHandDetectNetv2] param is illegal in func ChangeCHW2HWC. src[{}] is nullptr or channels[{}] "
            "<= 0 or height[{}] <= 0 or width[{}] <= 0 or dest.size[{}] not equal to total_elements[{}]",
            (void *)src, channels, height, width, dest.size(), total_elements);
        return false;
    }

    // 计算每个维度的跨度
    const int HxW = height * width;
    const int WxC = width * channels;

    for (int hi = 0; hi < height; hi++) {
        for (int wi = 0; wi < width; wi++) {
            for (int ci = 0; ci < channels; ci++) {
                int src_idx = ci * HxW + hi * width + wi;
                int dest_idx = hi * WxC + wi * channels + ci;

                // 确保索引在有效范围内
                if (src_idx >= total_elements || dest_idx >= total_elements) {
                    AISDK_LOG_ERROR(
                        "[ArtosynHandDetectNetv2] Index out of bounds: src_idx[{}] should < total_elements[{}] and  "
                        "dest_idx[{}] should < total_elements[{}]",
                        src_idx, total_elements, dest_idx, total_elements);
                    return false;
                }

                // 复制数据
                dest[dest_idx] = src[src_idx];
            }
        }
    }

    return true;
}

/// @brief 初始化手势检测网络
/// @param algo 算法配置参数（后处理参数等）
/// @param model 模型配置（模型路径，输入输出规格等）
/// @param session 会话配置（批量大小，设备选择等）
/// @return absl::Status 返回初始化状态
absl::Status ArtosynHandDetectNetv2::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                          aisdk::xengine::SessionConfig &session) {
    // step1: 批量处理策略配置
    if (model.dont_batch && session.batch > 1) {
        m_session_batch = session.batch;
        m_net_batch1 = true;
        session.batch = 1;
    }

    // step2: 调用基类初始化方法，完成公共网络参数设置
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        AISDK_LOG_ERROR("calculatorbasenet init failed");
        return ret;
    }

    ArtosynHandDetectNetV2reset();

    // step3：获取输入层索引并解析输入尺寸
    // int index_images = m_net->GetInputTensorIndex("images");
    aisdk::xengine::ArtosynTensorDims &input_dims = itensor.m_tensors[0].m_artosyn_dims;
    uint32_t height = input_dims.u32Height;  // 输入高度, 256
    uint32_t width = input_dims.u32Width;    // 输入宽度, 192

    AISDK_LOG_TRACE("artosyn detect input height[{}], width[{}]", height, width);

    // 计算特征网格尺寸（输入尺寸/网格步长）
    m_grid_h = height / m_grid_stride;
    m_grid_w = width / m_grid_stride;

    // step5: 预分配锚点容器空间（网格总数 = 行数×列数）
    m_grid_anchor.resize(m_grid_h * m_grid_w);

    // step6: 生成网格锚点系统
    for (auto i = 0; i < m_grid_h; i++) {
        for (auto j = 0; j < m_grid_w; j++) {
            // 计算网格中心坐标（归一化坐标，原点在图像中心）
            m_grid_anchor[i * m_grid_w + j].grid_x = -0.5f + j * 1.0f;  // X轴中心位置
            m_grid_anchor[i * m_grid_w + j].grid_y = -0.5f + i * 1.0f;  // Y轴中心位置

            // 设置锚点基准尺寸（基于典型手部尺寸）
            m_grid_anchor[i * m_grid_w + j].anchor_rw = 33.0f;  // 参考宽度（像素）
            m_grid_anchor[i * m_grid_w + j].anchor_rh = 30.0f;  // 参考高度（像素）
        }
    }

    // step6: 性能相关记录
    auto &prof = aisdk::base::DebugProfiling::Get().GetOpt();
    m_export_netalgo_exec_info = prof.export_pipeline_exec_info_jsonstring;
    AISDK_LOG_TRACE("ArtosynHandDetectNetv2::Inference  m_export_netalgo_exec_info={}", m_export_netalgo_exec_info);
    return ret;
}

/**
 * @brief 执行批量输入预处理
 * @param net_input 输入图像集合（多批次/多路输入）
 */
void ArtosynHandDetectNetv2::PreProcess(const std::vector<Image> &net_input) {
    AISDK_LOG_TRACE("ArtosynHandDetectNetv2::PreProcess");

    // step1: 检查输入数量与模型配置是否匹配（输入数量必须匹配且模型已配置批处理模式）
    int ai = itensor.m_batch * itensor.m_multishape_num;  // 模型预期输入数量 = 批次大小 × 多形状数
    int bi = net_input.size();                            // 实际输入图像数量
    if (ai != bi || itensor.m_packed_bybatch == false) {
        AISDK_LOG_ERROR(
            "ArtosynHandDetectNetv2::PreProcess ai[{}] not equal to bi[{}] or m_apcked_bybatch[{}] is false, do not do "
            "preprocess. itensor.m_batch[{}], "
            "itensor.m_multishape_num[{}]",
            ai, bi, itensor.m_packed_bybatch, itensor.m_batch, itensor.m_multishape_num);
        return;
    }

    // step2: 遍历处理每张输入图像
    for (int i = 0; i < bi; i++) {
        auto &img = net_input[i].m_mat;

        // step3: 计算张量索引位置
        int multi_i = i / itensor.m_batch;  // 多形状索引（支持多种输入形状）
        int batch_i = i % itensor.m_batch;  // 批次索引（当前批次中的位置）

        // step4: 解析输入尺寸
        int channels = itensor.m_tensors[multi_i].m_artosyn_dims.u32OriChannels;
        int height = itensor.m_tensors[multi_i].m_artosyn_dims.u32Height;
        int width = itensor.m_tensors[multi_i].m_artosyn_dims.u32Width;

        // step5: 准备张量内存信息
        int element_byte = itensor.m_tensors[multi_i].m_elementbyte;  // 张量元素字节大小
        int mem_size = height * width * channels * element_byte;      // 单张输入所需内存大小
        char *mem = (char *)itensor.m_tensors[multi_i].m_viraddr + batch_i * mem_size;  // 当前输入的目标内存地址

        // step6: 记录原始图像尺寸（用于后处理阶段坐标映射）
        m_origin_img_width = img.cols;   // 原始图像宽度
        m_origin_img_height = img.rows;  // 原始图像高度

        // step7: 计算宽高缩放比例
        float wratio = float(width) / float(m_origin_img_width);    // 宽度缩放比例
        float hratio = float(height) / float(m_origin_img_height);  // 高度缩放比例

        // step8: 计算最佳缩放比例并选择插值方法
        float ratio = std::min(wratio, hratio);                        // 取最小缩放比例（保持宽高比）
        int tmp = (ratio < 1.0f) ? cv::INTER_AREA : cv::INTER_LINEAR;  // 缩小用区域插值，放大用线性插值

        // step9: 图像resize
        cv::Mat image_resized(cv::Size(width, height), CV_8UC1);  // 创建临时8位图像
        cv::resize(img, image_resized, cv::Size(width, height), 0, 0, tmp);

        // step10: 数据拷贝
        cv::Mat new_mat(cv::Size(width, height), CV_8UC1, mem);  // 创建目标内存包装矩阵
        image_resized.copyTo(new_mat);
    }
}

/**
 * @brief 执行检测结果后处理
 * @param result 输出结果容器（包含左右手检测框信息）
 */
void ArtosynHandDetectNetv2::PostProcess(DetOutputInternal &result) {
    AISDK_LOG_TRACE("ArtosynHandDetectNetv2::PostProcess");

    // step1: 检查输出tensor支持批处理
    if (otensor.m_packed_bybatch == false) {
        AISDK_LOG_ERROR("[ArtosynHandDetectNetv2] postprocess otensor.m_packed_bybatch is not true");
        return;
    }

    // step2: 初始化结果容器
    result.images_lhand_rects.resize(otensor.m_batch);
    result.images_rhand_rects.resize(otensor.m_batch);

    // step3: 获取输出张量索引
    int index_box = 0;
    int index_cls = 1;

    // step4: 批次循环处理，目前不支持多batch，batch始终为1
    for (int batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
        // int box_n = otensor.m_tensors[index_box].m_artosyn_dims.u32Num;
        int box_c = otensor.m_tensors[index_box].m_artosyn_dims.u32OriChannels;
        int box_h = otensor.m_tensors[index_box].m_artosyn_dims.u32Height;
        int box_w = otensor.m_tensors[index_box].m_artosyn_dims.u32Width;
        // int box_begin_index = 32 * 1024 * 1024 * otensor.m_tensors[index_box].m_artosyn_dims.u32Bank +
        // otensor.m_tensors[index_box].m_artosyn_dims.u32Offset; const size_t box_total_size = box_n * box_c * box_h *
        // box_w;

        // int cls_n = otensor.m_tensors[index_cls].m_artosyn_dims.u32Num;
        int cls_c = otensor.m_tensors[index_cls].m_artosyn_dims.u32OriChannels;
        int cls_h = otensor.m_tensors[index_cls].m_artosyn_dims.u32Height;
        int cls_w = otensor.m_tensors[index_cls].m_artosyn_dims.u32Width;
        // int cls_begin_index = 32 * 1024 * 1024 * otensor.m_tensors[index_cls].m_artosyn_dims.u32Bank +
        // otensor.m_tensors[index_cls].m_artosyn_dims.u32Offset; const size_t cls_total_size = cls_n * cls_c * cls_h *
        // cls_w;

        // step5: 计算当前输出尺度下的内存起始位置，并获取数据内容到box_data和cls_data中
        int box_element_byte = otensor.m_tensors[index_box].m_elementbyte;
        char *box_mem =
            (char *)otensor.m_tensors[index_box].m_viraddr + batch_i * box_h * box_w * box_c * box_element_byte;
        float *box_data_origin = (float *)box_mem;
        std::vector<float> box_data(box_c * box_h * box_w, 0.0f);
        if (true != ChangeCHW2HWC(box_data_origin, box_data, box_c, box_h, box_w)) {
            continue;
        }

        int cls_element_byte = otensor.m_tensors[index_cls].m_elementbyte;
        char *cls_mem =
            (char *)otensor.m_tensors[index_cls].m_viraddr + batch_i * cls_h * cls_w * cls_c * cls_element_byte;
        float *cls_data_origin = (float *)cls_mem;
        std::vector<float> cls_data(cls_c * cls_h * cls_w, 0.0f);
        if (true != ChangeCHW2HWC(cls_data_origin, cls_data, cls_c, cls_h, cls_w)) {
            continue;
        }

        // int box_num = box_c * box_h * box_w;
        // AISDK_LOG_TRACE("ArtosynHandDetectNetv2::PostProcess, begin to output box info, box_num[{}]", box_num);
        // for (int i = 0; i < box_num; i++) {
        //     AISDK_LOG_TRACE("ArtosynHandDetectNetv2::PostProcess, i[{}], box_data[{}]", i, box_data[i]);
        // }

        // AISDK_LOG_TRACE("ArtosynHandDetectNetv2::PostProcess, end to output box info");

        // int cls_num = cls_c * cls_h * cls_w;
        // AISDK_LOG_TRACE("ArtosynHandDetectNetv2::PostProcess, begin to output cls info, cls_num[{}]", cls_num);
        // for (int i = 0; i < cls_num; i++) {
        //     AISDK_LOG_TRACE("ArtosynHandDetectNetv2::PostProcess, i[{}], cls_data[{}]", i, cls_data[i]);
        // }

        // AISDK_LOG_TRACE("ArtosynHandDetectNetv2::PostProcess, end to output cls info");

        // if (box_c != FEATURE_BOX_NUM || cls_c != FEATURE_CLS_NUM || box_h !=
        // m_grid_h || box_w != m_grid_w ||
        //     cls_h != m_grid_h || cls_w != m_grid_w) {
        //     continue;
        // }

        AISDK_LOG_TRACE("ArtosynHandDetectNetv2::Get Results");
        AISDK_LOG_TRACE("ArtosynHandDetectNetv2:: cls_c[{}], cls_h[{}], cls_w[{}], box_c[{}], box_h[{}], bow_w[{}]",
                        cls_c, cls_h, cls_w, box_c, box_h, box_w);

        // step6: 特征图遍历，遍历所有网格单元，获取合理结果并保存到tmp_result中
        std::vector<DetectRect> tmp_result;                // 临时存储所有检测框
        for (int idx_i = 0; idx_i < cls_h; idx_i++) {      // y坐标遍历
            for (int idx_j = 0; idx_j < cls_w; idx_j++) {  // x坐标遍历
                // 计算不同属性的索引位置
                int cls_idx_group = idx_i * cls_w * cls_c + idx_j * cls_c;
                int cls_idx_score = cls_idx_group;      // 置信度分数位置
                int cls_idx_left = cls_idx_group + 1;   // 左手置信度位置
                int cls_idx_right = cls_idx_group + 2;  // 右手置信度位置

                // 当前网格单元的置信度分数
                float obj_score = cls_data[cls_idx_score];
                float left_score = cls_data[cls_idx_left] * obj_score;
                float right_score = cls_data[cls_idx_right] * obj_score;

                // 判断是左手还是右手（根据左右手置信度的加权分数）
                bool is_left = left_score > right_score;
                float score = is_left ? left_score : right_score;

                // AISDK_LOG_TRACE(
                //     "cls_idx_score[{}], cls_idx_left[{}], cls_idx_right[{}], score[{}], left_score[{}], "
                //     "right_score[{}], obj_score[{}], score_threshold[{}]",
                //     cls_idx_score, cls_idx_left, cls_idx_right, score, left_score, right_score, obj_score,
                //     m_score_threshold);
                // 如果置信度超过阈值，则处理该检测结果
                if (score > m_score_threshold) {
                    float _coord[box_c];

                    // 进行这组结果的坐标提取
                    int box_idx_group = idx_i * box_w * box_c + idx_j * box_c;
                    for (int i = 0; i < 4; i++) {
                        _coord[i] = box_data[box_idx_group + i];
                    }

                    // 计算当前网格在特征图中的索引
                    uint32_t grid_anchor_index = idx_i * box_w + idx_j;

                    /**
                     * 创建检测矩形对象并填充信息(xywh)
                     * 计算宽度(w)：公式源于网络设计 (pow(coord[2]*2, 2) * 锚点基准宽)
                     * 计算高度(h)：公式源于网络设计 (pow(coord[3]*2, 2) * 锚点基准高)
                     * 计算中心点x坐标：(coord[0]*2 + 网格x位置) * 网格步长 - 宽度/2
                     * 计算中心点y坐标：(coord[1]*2 + 网格y位置) * 网格步长 - 高度/2
                     */
                    DetectRect tmp;
                    tmp.w = std::pow(_coord[2] * 2, 2) * m_grid_anchor[grid_anchor_index].anchor_rw;
                    tmp.h = std::pow(_coord[3] * 2, 2) * m_grid_anchor[grid_anchor_index].anchor_rh;
                    tmp.x = (_coord[0] * 2 + m_grid_anchor[grid_anchor_index].grid_x) * m_grid_stride - tmp.w / 2;
                    tmp.y = (_coord[1] * 2 + m_grid_anchor[grid_anchor_index].grid_y) * m_grid_stride - tmp.h / 2;

                    // 其他属性信息
                    tmp.confidence = score;                          // 置信度分数
                    tmp.left_confidence = cls_data[cls_idx_left];    // 左手置信度
                    tmp.right_confidence = cls_data[cls_idx_right];  // 右手置信度
                    tmp.is_left = is_left;                           // 是否左右手
                    tmp.nms_suppressed = false;                      // NMS标记初始化为未抑制
                    tmp_result.emplace_back(tmp);
                }
            }
        }

        AISDK_LOG_TRACE("ArtosynHandDetectNetv2::tmp_result size(before nms): {}", tmp_result.size());

        auto &lhand_rect = result.images_lhand_rects[batch_i];
        auto &rhand_rect = result.images_rhand_rects[batch_i];

        // step7: 对获取的临时结果，应用非极大值抑制(NMS)，消除重叠框
        nms(tmp_result, m_iou_threshold);
        AISDK_LOG_TRACE("ArtosynHandDetectNetv2::tmp_result size(after nms): {}", tmp_result.size());

        // step8: 获取不同输入格式下的height和width，并进一步计算输入张量与原始图像的缩放比例和填充
        int height = itensor.m_tensors[0].m_artosyn_dims.u32Height;
        int width = itensor.m_tensors[0].m_artosyn_dims.u32Width;

        // 计算输入张量和原始图像的缩放比例和填充
        float min_ratio =
            std::min(float(width) / float(m_origin_img_width), float(height) / float(m_origin_img_height));
        float padx = (float(width) - float(m_origin_img_width) * min_ratio) / 2;
        float pady = (float(height) - float(m_origin_img_height) * min_ratio) / 2;

        // step9: 遍历经过NMS后的检测框，获取一组最终左右手的结果，填充到lhand_rect和rhand_rect中
        for (auto &iter : tmp_result) {
            if (iter.nms_suppressed) {
                continue;
            }

            // 将框坐标从网络输入尺寸转换回原始图像尺寸
            iter.x = std::round((iter.x - padx) / min_ratio);
            iter.y = std::round((iter.y - pady) / min_ratio);
            iter.w = std::round(iter.w / min_ratio);
            iter.h = std::round(iter.h / min_ratio);

            if (true == iter.is_left && lhand_rect.size() == 0) {
                AISDK_LOG_TRACE("push lhand rect: x: {}, y: {}, w: {}, h: {}", iter.x, iter.y, iter.w, iter.h);
                lhand_rect.push_back(iter);
            } else if (false == iter.is_left && rhand_rect.size() == 0) {
                AISDK_LOG_TRACE("push rhand rect: x: {}, y: {}, w: {}, h: {}", iter.x, iter.y, iter.w, iter.h);
                rhand_rect.push_back(iter);
            } else {
                // do nothing
            }
        }
    }
}

/**
 * @brief 执行手部检测推理流程
 * @param baseinput 输入图像集合（支持多批次输入）
 * @param baseresult 输出检测结果容器
 * @return absl::Status 返回推理状态（包含错误信息）
 */
absl::Status ArtosynHandDetectNetv2::Inference(const std::vector<Image> &baseinput, DetOutputInternal &baseresult) {
    absl::Status ret;
    if (m_net_batch1) {  // 单批次处理
        AISDK_LOG_ERROR(
            "[ArtosynHandDetectNetv2] Inference failed, not support ArtosynHandDetectNetv2::Inference single batch "
            "branch");
        return absl::UnavailableError("failed to do inference in ArtosynHandDetectNetv2");
    } else {  // 批量处理
        PreProcess(baseinput);
        ret = m_net->RunNet();
        if (ret.ok()) {
            PostProcess(baseresult);

            AISDK_LOG_TRACE("[ArtosynHandDetectNetv2::Inference] baseresult.images_lhand_rects[0].size(): {}",
                            baseresult.images_lhand_rects[0].size());
            AISDK_LOG_TRACE("[ArtosynHandDetectNetv2::Inference] baseresult.images_lhand_rects[1].size(): {}",
                            baseresult.images_lhand_rects[1].size());
            AISDK_LOG_TRACE("[ArtosynHandDetectNetv2::Inference] baseresult.images_rhand_rects[0].size(): {}",
                            baseresult.images_rhand_rects[0].size());
            AISDK_LOG_TRACE("[ArtosynHandDetectNetv2::Inference] baseresult.images_rhand_rects[1].size(): {}",
                            baseresult.images_rhand_rects[1].size());
        } else {
            // 失败时清空容器（防止脏数据）
            baseresult.images_lhand_rects.resize(otensor.m_batch);
            baseresult.images_rhand_rects.resize(otensor.m_batch);
            AISDK_LOG_TRACE("ArtosynHandDetectNetv2::Inference  Error!");
        }
    }

    return ret;
}

}  // namespace aisdk::algorithm
