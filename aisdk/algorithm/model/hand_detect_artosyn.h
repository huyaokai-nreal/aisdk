#pragma once

#include "aisdk/xengine/nr_model_mgr.h"
#include "aisdk/algorithm/model/calculator_basenet.h"

#define FEATURE_NUM 7

namespace aisdk::algorithm {

/**
 * @class ArtosynHandDetectNetv2
 * @brief ARTOSYN手部检测专用神经网络实现（v2版本）
 * @details 继承自通用检测基类DetectBaseNet，实现针对手部检测的优化逻辑
 *          包含网格锚点机制、多分辨率适配、批量处理策略等增强特性
 *          Det v2.0 从单输出变成双输出
 *          - output_cls [B, 3, 16, 12] (3维依次是conf, left_cls, right_cls)
 *          - output_box [B, 4, 16, 12] (4维依次是x1,y1,x2,y2)
 * 
 * @note 主要特性：
 * - 支持单帧/批量输入处理
 * - 自适应输入图像宽高比
 * - 内置基于统计的锚点参数
 * - 可配置的检测阈值参数
 */
class ArtosynHandDetectNetv2 : public DetectBaseNet {
public:
    /**
     * @struct GridAnchor
     * @brief 网格锚点描述结构体
     * @details 定义检测网格系统中每个单元的参考位置和基础尺寸
     * 
     * 坐标系说明：
     * - 原点(0,0)对应输入图像中心
     * - 网格坐标基于归一化后的特征图尺寸
     */
    struct GridAnchor {
        // 显式定义默认构造函数
        GridAnchor() : grid_x(0), grid_y(0), anchor_rw(0), anchor_rh(0) {}  // 默认构造初始化为原点
        
        float grid_x;     // 网格单元中心x坐标
        float grid_y;     // 网格单元中心y坐标
        float anchor_rw;  // 预设瞄点参考宽度（单位：像素，基于训练数据统计）
        float anchor_rh;  // 预设瞄点参考高度（单位：像素，基于训练数据统计）
    };

    ArtosynHandDetectNetv2() : DetectBaseNet(){};  // 默认构造继承基类初始化
    ~ArtosynHandDetectNetv2() {};                  // 析构函数

    // 初始化
    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);

    // 模型推理
    virtual absl::Status Inference(const std::vector<Image> &baseinput, DetOutputInternal &baseresult) override;

private:
    void PreProcess(const std::vector<Image> &net_input);  // 批量预处理
    void PostProcess(DetOutputInternal &result);           // 批量后处理

    void PreProcessSingle(const std::vector<Image> &net_input, uint32_t batchn) {};   // 单样本预处理
    void PostProcessSingle(DetOutputInternal &result, uint32_t batchn) {};           // 单样本后处理
    void ArtosynHandDetectNetV2reset();

    aisdk::xengine::TensorFormat m_itensor_format;
    aisdk::xengine::TensorFormat m_otensor_format;

    std::vector<GridAnchor> m_grid_anchor;  // 网络瞄点集合（按行优先顺序进行存储）
    int m_origin_img_width;   // 原始输入图像宽度（预处理前）
    int m_origin_img_height;  // 原始输入图像高度（预处理后）

    // 检测阈值参数
    float m_score_threshold = 0.5f;   // 置信度阈值（默认0.5，过滤低质量检测）
    float m_iou_threshold = 0.45f;    // NMS重叠阈值（默认0.45，抑制重复框） 

    // 网格系统参数
    uint32_t m_grid_stride = 16;      // 网格步长（单位：像素。对应特征图下的采样率）
    uint32_t m_grid_w = 16;           // 水平方向网格数（默认16，适合640宽输入）
    uint32_t m_grid_h = 12;           // 垂直方向网格数（默认12，适合480高输入）

    // 批量处理控制
    bool m_net_batch1 = false;          // 单批次模式标志（当模型不支持批量时启用）
    uint32_t m_session_batch = 1;     // 用户设置的原始批量值（用于结果重组）

    // 执行信息导出标志（用于调试追踪）
    bool m_export_netalgo_exec_info = false;
};

}  // namespace aisdk::algorithm
