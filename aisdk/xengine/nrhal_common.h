#ifndef _NR_HAL_H_
#define _NR_HAL_H_

#include "nrhal_algop.h"
#include "nrhal_define.h"
#include "nrnn_model.h"
#include "nrnn_session.h"
// #include "nrtime.h"

void PrintfHalModelConfig(Xengine::ModelConfig& info);
void PrintfHalSessionConfig(Xengine::SessionConfig& info);
std::string DumpHalTensorMem(Xengine::Tensor& info);
void PrintfHalTensor(Xengine::Tensor& info);
void PrintfHalIoTensors(Xengine::IoTensors& info);
// 检查snpe batch异常的bug
bool CheckEngineSingleBatch(Xengine::VendorType& vendor);

// void HalInitDebugTRecord();
// NrUtils::TRecord* HalGetDebugTRecord(const char* time_name);
// void HalShowDebugTRecord(int interval);

extern "C" {

typedef Xengine::PlatformStatus* (*GetPlatformStatusFunc)();
SYM_EXPORT Xengine::PlatformStatus* _ZN2NR200TK7FUNC001E();
}

#endif