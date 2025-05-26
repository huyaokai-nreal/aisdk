#include "hand_detect_artosyn.h"

#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/base/profiling.h"
#include "aisdk/xengine/cv/xr_cv.h"
#include "aisdk/xengine/nrhal_common.h"

namespace aisdk::algorithm {

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

    // step3：获取输入层索引并解析输入尺寸
    int index_images = m_net->GetInputTensorIndex("images");
    aisdk::xengine::ArtosynTensorDims &input_dims = itensor.m_tensors[index_images].m_artosyn_dims;
    uint32_t height = input_dims.u32Height;  // 输入高度, 256
    uint32_t width = input_dims.u32Width;    // 输入宽度, 192
    AISDK_LOG_TRACE("detect artosyn input height[{}], width[{}]", height, width);

    // 当输入为竖屏比例（高>宽）时，调整网格划分密度（预设场景：480x640分辨率（竖屏手机拍摄））
    if (height > width) {
        m_grid_w = 12;  // 水平方向划分12个网格单元（对应640/16=40像素每格）
        m_grid_h = 16;  // 垂直方向划分16个网格单元（对应480/16=30像素每格）
    }

    // step4: 预分配锚点容器空间（网格总数 = 行数×列数）
    m_grid_anchor.resize(m_grid_h * m_grid_w);

    // step5: 遍历网格系统生成锚点参数
    for (auto i = 0; i < m_grid_h; i++) {
        for (auto j = 0; j < m_grid_w; j++) {
            auto &anchor = m_grid_anchor[i * m_grid_w + j];  // 当前锚点引用

            // 计算网格中心相对坐标（归一化坐标系，原点在图像中心）
            anchor.grid_x = -0.5f + j * 1.0f;  // X坐标：从-0.5开始，步长1.0
            anchor.grid_y = -0.5f + i * 1.0f;  // Y坐标：从-0.5开始，步长1.0

            // 预设锚点尺寸（基于典型手部检测框统计）
            anchor.anchor_rw = 31.0f;  // 预设宽度参考值（单位：像素）
            anchor.anchor_rh = 68.0f;  // 预设高度参考值（单位：像素）
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
 * @note 核心预处理流程：
 * 1. 输入数据合规性检查
 * 2. 遍历处理每个输入样本
 * 3. 尺寸调整与内存布局适配
 * 4. 数据格式转换（当前仅支持GRAY格式）
 */
void ArtosynHandDetectNetv2::PreProcess(const std::vector<Image> &net_input) {
    // step1：校验输入批量与模型配置的一致性
    int ai = itensor.m_batch * itensor.m_multishape_num;  // 预期输入：批次数 * 多路输入数
    int bi = net_input.size();                            // 实际输入数量
    if (ai != bi || itensor.m_packed_bybatch == false) {
        return;
    }

    // 获取input dims的部分属性
    int index_images = m_net->GetInputTensorIndex("images");
    aisdk::xengine::ArtosynTensorDims &input_dims = itensor.m_tensors[index_images].m_artosyn_dims;
    int height = input_dims.u32Height;  // 目标高度
    int width = input_dims.u32Width;    // 目标宽度

    // step2：遍历处理每个输入图像
    for (int i = 0; i < bi; i++) {
        // step2.1：获取当前图像引用（opencv矩阵格式）
        auto &img = net_input[i].m_mat;

        // step2.2：获取内存布局参数
        int width_s = width;  // itensor.m_tensors[i].m_wstride;  // 内存步幅（考虑对齐填充）暂时这么写

        // step2.3：获取内存地址指针
        char *mem = (char *)itensor.m_tensors[i].m_viraddr;

        // 原始尺寸记录
        m_origin_img_width = img.cols;
        m_origin_img_height = img.rows;

        // 计算宽高缩放比例
        float wratio = float(width) / float(m_origin_img_width);
        float hratio = float(height) / float(m_origin_img_height);

        // 选择最优缩放策略
        float ratio = std::min(wratio, hratio);                        // 保持长宽比的缩放比例
        int tmp = (ratio < 1.0f) ? cv::INTER_AREA : cv::INTER_LINEAR;  // 下采样用AREA, 上采样用LINEAR

        // step2.4：执行内存拷贝（考虑内存对齐）
        if (width == width_s) {  // 无内存步幅的特殊处理
            // 直接创建目标尺寸的opencv矩阵，执行resize操作（直接写入设备内存）
            cv::Mat image_resized(cv::Size(width, height), CV_8UC1, mem);
            cv::resize(img, image_resized, cv::Size(width, height), 0, 0, tmp);
        } else {  // 存在内存对齐步幅的处理
            // 创建带步幅的目标矩阵，在有效区域内执行resize（避免写入填充区域）
            cv::Mat dst_resized(cv::Size(width_s, height), CV_8UC1, mem);
            cv::resize(img, dst_resized(cv::Rect(0, 0, width, height)), cv::Size(width, height), 0, 0, tmp);
        }
    }
}

/**
 * @brief 执行检测结果后处理
 * @param result 输出结果容器（包含左右手检测框信息）
 * @note 处理流程：
 * 1. 初始化结果容器
 * 2. 获取网络输出张量信息
 * 3. 遍历所有网格位置解析检测结果
 * 4. 应用非极大值抑制(NMS)
 * 5. 坐标映射回原始图像空间
 * 6. 分类存储左右手检测结果
 */
void ArtosynHandDetectNetv2::PostProcess(DetOutputInternal &result) {
    // step1: 初始化结果容器，按批次大小预分配左右手结果容器
    result.images_lhand_rects.resize(otensor.m_batch);
    result.images_rhand_rects.resize(otensor.m_batch);

    // step2: 获取输出张量信息
    int index_box = m_net->GetOutputTensorIndex("output_box");  // 获取output_box输出张量索引
    int index_cls = m_net->GetOutputTensorIndex("output_cls");  // 获取ouput_cls输出张量索引

    // 解析张量维度信息
    aisdk::xengine::ArtosynTensorDims &box_dims = otensor.m_tensors[index_box].m_artosyn_dims;
    aisdk::xengine::ArtosynTensorDims &cls_dims = otensor.m_tensors[index_cls].m_artosyn_dims;
    int box_c = box_dims.u32OriChannels;                                // 边界框通道数（应为4: x,y,w,h）
    int box_h = box_dims.u32Height;                                     // 特征图高度（对应网格行数）
    int box_w = box_dims.u32Width;                                      // 特征图宽度（对应网格列数）
    float *box_data = (float *)otensor.m_tensors[index_box].m_viraddr;  // 边界框数据指针

    int cls_c = cls_dims.u32OriChannels;  // 分类通道数（应为3: 置信度 + 左右手概率）
    int cls_h = cls_dims.u32Height;       // 特征图高度（与box_h一致）
    int cls_w = cls_dims.u32Width;        // 特征图宽度（与box_w一致）
    float *cls_data = (float *)otensor.m_tensors[index_cls].m_viraddr;  // 分类数据指针

    // step3: 批次遍历处理
    for (int batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
        AISDK_LOG_TRACE("ArtosynHandDetectNetv2::Get Results");
        AISDK_LOG_TRACE("ArtosynHandDetectNetv2v2:: cls_c: {}, cls_h: {}, cls_w: {}, box_c: {}, box_h: {}, bow_w: {}",
                        cls_c, cls_h, cls_w, box_c, box_h, box_w);

        // 临时结果缓存
        std::vector<DetectRect> tmp_result;
        int cls_idx_group = 0;
        int cls_idx_score = 0;
        int cls_idx_left = 0;
        int cls_idx_right = 0;
        int box_idx_group = 0;

        // step4: 网格遍历，解析原始输出
        for (int idx_i = 0; idx_i < cls_h; idx_i++) {      // 行遍历（y轴）
            for (int idx_j = 0; idx_j < cls_w; idx_j++) {  // 列遍历（x轴）
                // 计算当前网络的分类数据索引
                cls_idx_score =
                    ArtosynNpuGetEntryIndex(batch_i, otensor.m_batch, idx_i, idx_j, 0, sizeof(float), cls_dims);
                cls_idx_left =
                    ArtosynNpuGetEntryIndex(batch_i, otensor.m_batch, idx_i, idx_j, 1, sizeof(float), cls_dims);
                cls_idx_right =
                    ArtosynNpuGetEntryIndex(batch_i, otensor.m_batch, idx_i, idx_j, 2, sizeof(float), cls_dims);

                // 获取置信度并判断左右手
                float score = cls_data[cls_idx_score];
                bool is_left = cls_data[cls_idx_left] * score > cls_data[cls_idx_right] * score;

                // 置信度阈值过滤
                if (score > m_score_threshold) {
                    // 获取对应边界框坐标数据
                    float _coord[box_c];
                    for (int i = 0; i < 4; i++) {
                        box_idx_group =
                            ArtosynNpuGetEntryIndex(batch_i, otensor.m_batch, idx_i, idx_j, i, sizeof(float), box_dims);
                        _coord[i] = box_data[box_idx_group];
                    }

                    // 计算当前网络对应的锚点索引
                    uint32_t grid_anchor_index = idx_i * box_w + idx_j;

                    /**
                     * Description: 生成检测框参数
                     * 宽度计算：(2*coord[2])^2 * 锚点参考宽
                     * 高度计算：(2*coord[3])^2 * 锚点参考高
                     * X坐标计算：(2*coord[0] + 网格x偏移) * 步长 - 半宽
                     * Y坐标计算：(2*coord[1] + 网格y偏移) * 步长 - 半高
                     */
                    DetectRect tmp;
                    tmp.w = std::pow(_coord[2] * 2, 2) * m_grid_anchor[grid_anchor_index].anchor_rw;
                    tmp.h = std::pow(_coord[3] * 2, 2) * m_grid_anchor[grid_anchor_index].anchor_rh;
                    tmp.x = (_coord[0] * 2 + m_grid_anchor[grid_anchor_index].grid_x) * m_grid_stride - tmp.w / 2;
                    tmp.y = (_coord[1] * 2 + m_grid_anchor[grid_anchor_index].grid_y) * m_grid_stride - tmp.h / 2;

                    // 置信度及分类信息记录
                    tmp.confidence = score;
                    tmp.left_confidence = cls_data[cls_idx_left];
                    tmp.right_confidence = cls_data[cls_idx_right];
                    tmp.is_left = is_left;
                    tmp.nms_suppressed = false;
                    tmp_result.emplace_back(tmp);
                }
            }
        }

        // step5: 非极大值抑制处理
        AISDK_LOG_TRACE("ArtosynHandDetectNetv2v2::tmp_result size(before nms): {}", tmp_result.size());
        auto &lhand_rect = result.images_lhand_rects[batch_i];
        auto &rhand_rect = result.images_rhand_rects[batch_i];
        nms(tmp_result, m_iou_threshold);  // 执行MNS算法
        AISDK_LOG_TRACE("ArtosynHandDetectNetv2v2::tmp_result size(after nms): {}", tmp_result.size());

        // 获取预处理后的网络输入尺寸
        int height = iImageblobs.m_imageblobs[0].m_height;
        int width = iImageblobs.m_imageblobs[0].m_width;

        // 计算预处理时的缩放比例和填充量
        float min_ratio =
            std::min(float(width) / float(m_origin_img_width), float(height) / float(m_origin_img_height));
        float padx = (float(width) - float(m_origin_img_width) * min_ratio) / 2;
        float pady = (float(height) - float(m_origin_img_height) * min_ratio) / 2;

        // step6: 遍历处理每个有效检测框
        for (auto &iter : tmp_result) {
            if (iter.nms_suppressed) {
                continue;
            }

            // 坐标反变换计算（去除填充并缩放回原图尺寸）
            iter.x = std::round((iter.x - padx) / min_ratio);
            iter.y = std::round((iter.y - pady) / min_ratio);
            iter.w = std::round(iter.w / min_ratio);
            iter.h = std::round(iter.h / min_ratio);

            // 分类存储左右手结果（当前实现每图只取最高置信度结果）
            if (true == iter.is_left && lhand_rect.size() == 0) {
                AISDK_LOG_TRACE("push lhand rect: x: {}, y: {}, w: {}, h: {}", iter.x, iter.y, iter.w, iter.h);
                lhand_rect.push_back(iter);
            } else if (false == iter.is_left && rhand_rect.size() == 0) {
                AISDK_LOG_TRACE("push rhand rect: x: {}, y: {}, w: {}, h: {}", iter.x, iter.y, iter.w, iter.h);
                rhand_rect.push_back(iter);
            }
        }
    }
}

void ArtosynHandDetectNetv2::PreProcessSingle(const std::vector<Image> &net_input, uint32_t batchn) {}

void ArtosynHandDetectNetv2::PostProcessSingle(DetOutputInternal &result, uint32_t batchn) {}

/**
 * @brief 执行手部检测推理流程
 * @param baseinput 输入图像集合（支持多批次输入）
 * @param baseresult 输出检测结果容器
 * @return absl::Status 返回推理状态（包含错误信息）
 *
 * @note 核心处理逻辑根据模型是否支持批量处理分为两种模式：
 * 1. 单批次模式（m_net_batch1=true）：拆分输入为单样本循环处理
 * 2. 正常批量模式：直接处理整个批量输入
 */
absl::Status ArtosynHandDetectNetv2::Inference(const std::vector<Image> &baseinput, DetOutputInternal &baseresult) {
    absl::Status ret;
    if (m_net_batch1) {  // 单批次处理
        // 结果容器预分配（根据用户设置的原始批量值）
        baseresult.images_lhand_rects.resize(m_session_batch);
        baseresult.images_rhand_rects.resize(m_session_batch);

        // 遍历处理每个输入样本（模拟批量处理）
        for (uint32_t i = 0; i < m_session_batch; i++) {
            PreProcessSingle(baseinput, i);
            ret = m_net->RunNet();
            if (ret.ok()) {
                PostProcessSingle(baseresult, i);
            } else {
                AISDK_LOG_TRACE("ArtosynHandDetectNetv2::Inference  Error!");
            }
        }
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
