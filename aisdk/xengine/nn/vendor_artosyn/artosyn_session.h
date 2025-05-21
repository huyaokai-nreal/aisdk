#ifndef _ARTOSYN_SESSION_H_
#define _ARTOSYN_SESSION_H_

#include "aisdk/xengine/nr_artosyn_header.h"
#include "aisdk/xengine/nrnn_session.h"
#include "artosyn_model.h"
namespace aisdk::xengine {

/**
 * @class ARTOSYN_Session
 * @brief ARTOSYN框架的会话管理类，继承自基础Session类
 * @details 负责模型加载、内存管理、推理执行等全生命周期管理
 *          包含NPU设备内存管理、运行时内存分配、输入输出张量构造等核心功能
 */
class ARTOSYN_Session : public Session {
public:
    ARTOSYN_Session();
    virtual ~ARTOSYN_Session();

    // 初始化，加载模型
    Status Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig);

    // 执行模型推理
    Status Forword(ModelInfo &handle);

private:
    //申请，释放npu输入/输出内存
    int MallocNPUBuff(void * handle, AR_U16 u16NetworkID);
    int FreeNPUBuff();

    // 申请，释放运行时内存
    int MallocRuntimeBuff(void * handle, AR_U16 u16NetworkID);
    int FreeRuntimeBuff();
    
    // 释放PCH协处理器内存
    int FreePchBuff();

    // 构造IFC输入张量
    int MakeIfcInput(std::shared_ptr<ARTOSYN_AIModel> &model);
    
    // 构造常规输入张量
    int MakeInput(std::shared_ptr<ARTOSYN_AIModel> &model);
    
    // 构造输出张量
    int MakeOutput(std::shared_ptr<ARTOSYN_AIModel> &model);

private:
    // npu内存状态标识
    bool m_blNPUInBuff = false;   // 输入内存分配状态标记
    AR_MEM_S m_stNPUInBuff;       // npu输入内存描述结构体（包含物理地址/大小等信息）
    bool m_blNPUOutBuff = false;  // 输出内存分配状态标记
    AR_MEM_S m_stNPUOutBuff;      // npu输出内存描述结构体
    bool m_blNPURtBuff = false;   // 运行时内存分配状态标记
    AR_MEM_S m_stNPURtBuff;       // 运行时内存描述结构体（包含权重/中间结果）

    // 开启ifc还需要额外的内存
    bool m_bEnable_ifc = false;
    AR_U32 u32FrameId = 0;        // 帧序列标识
    AR_IMG_SET_S m_stImg;         // 图像数据集结构
};

}  // namespace aisdk::xengine
#endif