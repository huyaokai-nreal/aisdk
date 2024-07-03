#ifndef _NR_HAL_H_
#define _NR_HAL_H_

#include "nrhal_algop.h"
#include "nrhal_define.h"
#include "nrnn_model.h"
#include "nrnn_session.h"
// #include "nrtime.h"
int ArtosynNpuGetEntryIndex(int batch, int h, int w, int c, int byteUnit, aisdk::xengine::ArtosynTensorDims &pTensorInfo);
absl::Status ConvertOldStatus(aisdk::xengine::Status old_status);
void PrintfHalModelConfig(aisdk::xengine::ModelConfig& info);
void PrintfHalSessionConfig(aisdk::xengine::SessionConfig& info);
std::string DumpHalTensorMem(aisdk::xengine::Tensor& info);
void PrintfHalTensor(aisdk::xengine::Tensor& info);
void PrintfHalIoTensors(aisdk::xengine::IoTensors& info);
// 检查snpe batch异常的bug
bool CheckEngineSingleBatch(aisdk::xengine::VendorType& vendor);

// void HalInitDebugTRecord();
// NrUtils::TRecord* HalGetDebugTRecord(const char* time_name);
// void HalShowDebugTRecord(int interval);

extern "C" {

typedef aisdk::xengine::PlatformStatus* (*GetPlatformStatusFunc)();
SYM_EXPORT aisdk::xengine::PlatformStatus* _ZN2NR200TK7FUNC001E();
}

#endif