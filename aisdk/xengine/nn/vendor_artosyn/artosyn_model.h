#ifndef _ARTOSYN_MODEL_H_
#define _ARTOSYN_MODEL_H_

#include "aisdk/xengine/nr_artosyn_header.h"
#include "aisdk/xengine/nrnn_model.h"

namespace aisdk::xengine {

/**
 * @class ARTOSYN_AIModel
 * @brief ARTOSYN专用AI模型实现类，继承自通用AIModel基类
 * @details 封装与ARTOSYN NPU芯片深度适配的模型属性和操作方法
 *          包含芯片版本识别、NPU资源管理、模型结构描述等核心功能
 */
class ARTOSYN_AIModel : public AIModel {
public:
    ARTOSYN_AIModel(ModelConfig &config);
    virtual ~ARTOSYN_AIModel();

    bool IsShared() { return false; }

public:
    AR_S32 m_socversion = 0;         // npu芯片版本标识（1:AR9341 2:AR9311 4:AR9481）
    AR_NPU_CNN_DESC_S m_stCNNDesc;   // cnn网络描述结构体（包含网络层数，各层类型，张量维度信息，量化参数等）
    void *m_handle = nullptr;        // npu设备操作句柄

    AR_U32 m_batch = 0;              // 批处理大小
    AR_U32 m_inputn = 0;             // 输入张量数量（对应模型输入层数量）
    AR_U32 m_outputn = 0;            // 输出张量数量（对应模型输出层数量）
};

}  // namespace aisdk::xengine

#endif
