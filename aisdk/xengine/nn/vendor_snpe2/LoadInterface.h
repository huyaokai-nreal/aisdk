/*
 * @Author: jszhang jszhang@nreal.ai
 * @Date: 2023-04-11 01:43:19
 * @LastEditors: jszhang jszhang@nreal.ai
 * @LastEditTime: 2023-04-12 05:32:27
 * @FilePath: /nreal_hand_demo_android/src/core/hal/nn/vendor_snpe/LoadInterface.h
 */
#ifndef _LOAD_INTERFACE_H_
#define _LOAD_INTERFACE_H_

#define DLOPEN_SNPE2

#ifdef DLOPEN_SNPE2
#include "aisdk/xengine/nr_snpe_header.h"

extern "C" {

#define REGISTER_C_INTERFACE_DECLARATION(RT, FUNNAME, ...) \
    typedef RT (*__##FUNNAME##__)(__VA_ARGS__);            \
    __##FUNNAME##__ FUNNAME;

typedef struct SnpeCInterface {
    /* SNPEUtil */
    REGISTER_C_INTERFACE_DECLARATION(int, Snpe_Util_IsRuntimeAvailable, Snpe_Runtime_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_DlVersion_Handle_t, Snpe_Util_GetLibraryVersion)
    REGISTER_C_INTERFACE_DECLARATION(int, Snpe_Util_IsRuntimeAvailableCheckOption, Snpe_Runtime_t,
                                     Snpe_RuntimeCheckOption_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_IUserBuffer_Handle_t, Snpe_Util_CreateUserBuffer, void*, size_t,
                                     Snpe_TensorShape_Handle_t, Snpe_IUserBuffer_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(int, Snpe_Util_InitializeLogging, Snpe_LogLevel_t)
    REGISTER_C_INTERFACE_DECLARATION(int, Snpe_Util_SetLogLevel, Snpe_LogLevel_t)
    REGISTER_C_INTERFACE_DECLARATION(int, Snpe_Util_TerminateLogging)
    /* RuntimeList */
    REGISTER_C_INTERFACE_DECLARATION(Snpe_RuntimeList_Handle_t, Snpe_RuntimeList_Create)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_RuntimeList_Add, Snpe_RuntimeList_Handle_t, Snpe_Runtime_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_RuntimeList_Delete, Snpe_RuntimeList_Handle_t)
    /* DlContainer */
    REGISTER_C_INTERFACE_DECLARATION(Snpe_DlContainer_Handle_t, Snpe_DlContainer_Open, const char*)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_DlContainer_Handle_t, Snpe_DlContainer_OpenBuffer, const uint8_t* buffer,
                                     const size_t size)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_DlContainer_Delete, Snpe_DlContainer_Handle_t)
    /* PlatformConfig */
    REGISTER_C_INTERFACE_DECLARATION(Snpe_PlatformConfig_Handle_t, Snpe_PlatformConfig_Create)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_PlatformConfig_Handle_t, Snpe_PlatformConfig_CreateCopy, Snpe_PlatformConfig_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_PlatformConfig_Delete, Snpe_PlatformConfig_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_PlatformConfig_PlatformType_t, Snpe_PlatformConfig_GetPlatformType, Snpe_PlatformConfig_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(int, Snpe_PlatformConfig_IsValid, Snpe_PlatformConfig_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(int, Snpe_PlatformConfig_SetPlatformOptions, Snpe_PlatformConfig_Handle_t, const char* options)
    REGISTER_C_INTERFACE_DECLARATION(const char*, Snpe_PlatformConfig_GetPlatformOptions, Snpe_PlatformConfig_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(int, Snpe_PlatformConfig_SetPlatformOptionValue, Snpe_PlatformConfig_Handle_t, const char* optionName, const char* value)
    REGISTER_C_INTERFACE_DECLARATION(int, Snpe_PlatformConfig_RemovePlatformOptionValue, Snpe_PlatformConfig_Handle_t, const char* optionName, const char* value)
    /* SNPEBuilder */
    REGISTER_C_INTERFACE_DECLARATION(Snpe_SNPEBuilder_Handle_t, Snpe_SNPEBuilder_Create, Snpe_DlContainer_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_SNPEBuilder_SetRuntimeProcessorOrder,
                                     Snpe_SNPEBuilder_Handle_t, Snpe_RuntimeList_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_SNPEBuilder_SetUseUserSuppliedBuffers,
                                     Snpe_SNPEBuilder_Handle_t, int)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_SNPEBuilder_SetPerformanceProfile,
                                     Snpe_SNPEBuilder_Handle_t, Snpe_PerformanceProfile_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_SNPEBuilder_SetOutputLayers, Snpe_SNPEBuilder_Handle_t,
                                     Snpe_StringList_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_SNPEBuilder_SetOutputTensors, Snpe_SNPEBuilder_Handle_t,
                                     Snpe_StringList_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_SNPEBuilder_SetInputDimensions, Snpe_SNPEBuilder_Handle_t,
                                     Snpe_TensorShapeMap_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_SNPEBuilder_SetPlatformConfig, Snpe_SNPEBuilder_Handle_t,
                                     Snpe_PlatformConfig_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_SNPE_Handle_t, Snpe_SNPEBuilder_Build, Snpe_SNPEBuilder_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_SNPEBuilder_Delete, Snpe_SNPEBuilder_Handle_t)
    /* SNPE */
    REGISTER_C_INTERFACE_DECLARATION(Snpe_StringList_Handle_t, Snpe_SNPE_GetInputTensorNames, Snpe_SNPE_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_IBufferAttributes_Handle_t, Snpe_SNPE_GetInputOutputBufferAttributes,
                                     Snpe_SNPE_Handle_t, const char*)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_StringList_Handle_t, Snpe_SNPE_GetOutputTensorNames, Snpe_SNPE_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_SNPE_ExecuteUserBuffers, Snpe_SNPE_Handle_t,
                                     Snpe_UserBufferMap_Handle_t, Snpe_UserBufferMap_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_SNPE_Delete, Snpe_SNPE_Handle_t);
    /* TensorShapeMap */
    REGISTER_C_INTERFACE_DECLARATION(Snpe_TensorShapeMap_Handle_t, Snpe_TensorShapeMap_Create)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_TensorShapeMap_Add, Snpe_TensorShapeMap_Handle_t,
                                     const char*, Snpe_TensorShape_Handle_t)
    /* TensorShape */
    REGISTER_C_INTERFACE_DECLARATION(Snpe_TensorShape_Handle_t, Snpe_TensorShape_CreateDimsSize, const size_t*, size_t)
    REGISTER_C_INTERFACE_DECLARATION(const size_t*, Snpe_TensorShape_GetDimensions, Snpe_TensorShape_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(size_t, Snpe_TensorShape_Rank, Snpe_TensorShape_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(size_t, Snpe_TensorShape_At, Snpe_TensorShape_Handle_t, size_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_TensorShape_Delete, Snpe_TensorShape_Handle_t)
    /* ErrorCode */
    REGISTER_C_INTERFACE_DECLARATION(const char*, Snpe_ErrorCode_GetLastErrorString)
    /* StringList */
    REGISTER_C_INTERFACE_DECLARATION(Snpe_StringList_Handle_t, Snpe_StringList_Create)
    REGISTER_C_INTERFACE_DECLARATION(size_t, Snpe_StringList_Size, Snpe_StringList_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(const char*, Snpe_StringList_At, Snpe_StringList_Handle_t, size_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_StringList_Append, Snpe_StringList_Handle_t, const char*)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_StringList_Delete, Snpe_StringList_Handle_t)
    /* UserBufferMap */
    REGISTER_C_INTERFACE_DECLARATION(Snpe_UserBufferMap_Handle_t, Snpe_UserBufferMap_Create)
    REGISTER_C_INTERFACE_DECLARATION(void, Snpe_UserBufferMap_Add, Snpe_UserBufferMap_Handle_t, const char*,
                                     Snpe_IUserBuffer_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_UserBufferMap_Delete, Snpe_UserBufferMap_Handle_t)
    /* IBufferAttributes */
    REGISTER_C_INTERFACE_DECLARATION(Snpe_TensorShape_Handle_t, Snpe_IBufferAttributes_GetDims,
                                     Snpe_IBufferAttributes_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_IBufferAttributes_Delete, Snpe_IBufferAttributes_Handle_t)
    /* IUserBuffer */
    REGISTER_C_INTERFACE_DECLARATION(Snpe_UserBufferEncoding_Handle_t, Snpe_UserBufferEncodingFloat_Create)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_UserBufferEncodingFloat_Delete,
                                     Snpe_UserBufferEncoding_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_IUserBuffer_Delete, Snpe_IUserBuffer_Handle_t)
    /* DlVersion */
    REGISTER_C_INTERFACE_DECLARATION(const char*, Snpe_DlVersion_ToString, Snpe_DlVersion_Handle_t)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_DlVersion_Delete, Snpe_DlVersion_Handle_t)
} SnpeCInterface;

typedef struct SnpePlatformCInterface {
    /* PlatformValidator */
    REGISTER_C_INTERFACE_DECLARATION(Snpe_PlatformValidator_Handle_t, Snpe_PlatformValidator_Create)
    REGISTER_C_INTERFACE_DECLARATION(void, Snpe_PlatformValidator_SetRuntime, Snpe_PlatformValidator_Handle_t,
                                     Snpe_Runtime_t, bool)
    REGISTER_C_INTERFACE_DECLARATION(int, Snpe_PlatformValidator_IsRuntimeAvailable, Snpe_PlatformValidator_Handle_t,
                                     bool)
    REGISTER_C_INTERFACE_DECLARATION(Snpe_ErrorCode_t, Snpe_PlatformValidator_Delete, Snpe_PlatformValidator_Handle_t)
} SnpePlatformCInterface;

// int LoadSnpe2CInterface(const char* snpe_soname, struct SnpeCInterface* api);
// int UnloadSnpe2CInterface();
}
#endif

#endif
