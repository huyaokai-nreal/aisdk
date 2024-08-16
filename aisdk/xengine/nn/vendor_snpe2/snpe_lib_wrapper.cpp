/*
 * @Author: jszhang jszhang@nreal.ai
 * @Date: 2023-04-10 05:29:03
 * @LastEditors: jszhang jszhang@nreal.ai
 * @LastEditTime: 2023-04-14 02:57:52
 * @FilePath: /SNPE_demo/src/snpe/snpe_lib_wrapper.cpp
 */
#include "snpe_lib_wrapper.h"

#include <stdlib.h>

#include <cstdint>
#include <sstream>

#include "aisdk/base/log.h"

int SNPELibWrapper::LoadSnpe2CInterface(const char* snpe_soname, struct SnpeCInterface* api) {
#define ConvertDlstruct(FUNNAME) \
    { (void**)&snpe2_provider_.FUNNAME, #FUNNAME }

    struct Dlstruct {
        void** p;
        const char* name;
    };

    struct Dlstruct dllist[] = {
        /* SNPEUtil */
        ConvertDlstruct(Snpe_Util_IsRuntimeAvailable), ConvertDlstruct(Snpe_Util_IsRuntimeAvailableCheckOption),
        ConvertDlstruct(Snpe_Util_GetLibraryVersion), ConvertDlstruct(Snpe_Util_CreateUserBuffer),
        ConvertDlstruct(Snpe_Util_InitializeLogging), ConvertDlstruct(Snpe_Util_SetLogLevel),
        ConvertDlstruct(Snpe_Util_TerminateLogging),
        /* RuntimeList */
        ConvertDlstruct(Snpe_RuntimeList_Create), ConvertDlstruct(Snpe_RuntimeList_Add),
        ConvertDlstruct(Snpe_RuntimeList_Delete),
        /* DlContainer */
        ConvertDlstruct(Snpe_DlContainer_Open), ConvertDlstruct(Snpe_DlContainer_OpenBuffer),
        ConvertDlstruct(Snpe_DlContainer_Delete),
        /* PlatformConfig */
        ConvertDlstruct(Snpe_PlatformConfig_Create), ConvertDlstruct(Snpe_PlatformConfig_CreateCopy),
        ConvertDlstruct(Snpe_PlatformConfig_Delete), ConvertDlstruct(Snpe_PlatformConfig_GetPlatformType),
        ConvertDlstruct(Snpe_PlatformConfig_IsValid), ConvertDlstruct(Snpe_PlatformConfig_SetPlatformOptions),
        ConvertDlstruct(Snpe_PlatformConfig_GetPlatformOptions),
        ConvertDlstruct(Snpe_PlatformConfig_SetPlatformOptionValue),
        ConvertDlstruct(Snpe_PlatformConfig_RemovePlatformOptionValue),
        /* SNPEBuilder */
        ConvertDlstruct(Snpe_SNPEBuilder_Create), ConvertDlstruct(Snpe_SNPEBuilder_SetRuntimeProcessorOrder),
        ConvertDlstruct(Snpe_SNPEBuilder_SetUseUserSuppliedBuffers),
        ConvertDlstruct(Snpe_SNPEBuilder_SetPerformanceProfile), ConvertDlstruct(Snpe_SNPEBuilder_SetInputDimensions),
        ConvertDlstruct(Snpe_SNPEBuilder_SetOutputLayers), ConvertDlstruct(Snpe_SNPEBuilder_SetOutputTensors),
        ConvertDlstruct(Snpe_SNPEBuilder_SetPlatformConfig), ConvertDlstruct(Snpe_SNPEBuilder_SetProfilingLevel),
        ConvertDlstruct(Snpe_SNPEBuilder_Build), ConvertDlstruct(Snpe_SNPEBuilder_Delete),
        /* SNPE */
        ConvertDlstruct(Snpe_SNPE_GetInputTensorNames), ConvertDlstruct(Snpe_SNPE_GetInputOutputBufferAttributes),
        ConvertDlstruct(Snpe_SNPE_GetOutputTensorNames), ConvertDlstruct(Snpe_SNPE_ExecuteUserBuffers),
        ConvertDlstruct(Snpe_SNPE_Delete), ConvertDlstruct(Snpe_SNPE_GetDiagLogInterface_Ref),
        /* TensorShapeMap */
        ConvertDlstruct(Snpe_TensorShapeMap_Create), ConvertDlstruct(Snpe_TensorShapeMap_Add),
        /* TensorShape */
        ConvertDlstruct(Snpe_TensorShape_CreateDimsSize), ConvertDlstruct(Snpe_TensorShape_GetDimensions),
        ConvertDlstruct(Snpe_TensorShape_Rank), ConvertDlstruct(Snpe_TensorShape_At),
        ConvertDlstruct(Snpe_TensorShape_Delete),
        /* ErrorCode */
        ConvertDlstruct(Snpe_ErrorCode_GetLastErrorString),
        /* StringList */
        ConvertDlstruct(Snpe_StringList_Create), ConvertDlstruct(Snpe_StringList_Size),
        ConvertDlstruct(Snpe_StringList_At), ConvertDlstruct(Snpe_StringList_Append),
        ConvertDlstruct(Snpe_StringList_Delete),
        /* UserBufferMap */
        ConvertDlstruct(Snpe_UserBufferMap_Create), ConvertDlstruct(Snpe_UserBufferMap_Add),
        ConvertDlstruct(Snpe_UserBufferMap_Delete),
        /* IBufferAttributes */
        ConvertDlstruct(Snpe_IBufferAttributes_GetDims), ConvertDlstruct(Snpe_IBufferAttributes_Delete),
        /* IUserBuffer */
        ConvertDlstruct(Snpe_UserBufferEncodingFloat_Create), ConvertDlstruct(Snpe_UserBufferEncodingFloat_Delete),
        ConvertDlstruct(Snpe_IUserBuffer_Delete),
        /* DlVersion */
        ConvertDlstruct(Snpe_DlVersion_ToString), ConvertDlstruct(Snpe_DlVersion_Delete),
        /* IDiagLog */
        ConvertDlstruct(Snpe_IDiagLog_GetOptions), ConvertDlstruct(Snpe_IDiagLog_SetOptions),
        ConvertDlstruct(Snpe_IDiagLog_Start), ConvertDlstruct(Snpe_IDiagLog_Stop),
        /* IDiagLog Options */
        ConvertDlstruct(Snpe_Options_Create), ConvertDlstruct(Snpe_Options_Delete),
        ConvertDlstruct(Snpe_Options_GetLogFileName), ConvertDlstruct(Snpe_Options_SetLogFileName),
        ConvertDlstruct(Snpe_Options_SetLogFileDirectory)};

    // std::stringstream path;
    // std::string native_lib_path =
    //     "/data/app/~~hqe2OtBmEMSp7WEgMSre3A==/ai.nreal.sdk.unity.HandTracking-zqo8xKQmRdPSKC1N-rEBmA==/lib/arm64";
    // path << native_lib_path << ";/system/lib/rfsa/adsp;/system/vendor/lib/rfsa/adsp;/dsp";
    // setenv("ADSP_LIBRARY_PATH", path.str().c_str(), 0 /*override*/);
    // AISDK_LOG_TRACE("SetAdspLibraryPath ok");

    if (!snpe2_handle_) {
        snpe2_handle_ = dlopen(snpe_soname, RTLD_LAZY);
        if (snpe2_handle_) {
            int error = 0;
            for (uint32_t i = 0; i < sizeof(dllist) / sizeof(Dlstruct); i++) {
                struct Dlstruct tmp = dllist[i];
                void* p1 = dlsym(snpe2_handle_, tmp.name);
                if (NULL == p1) {
                    const char* dlsym_error = dlerror();
                    AISDK_LOG_ERROR("dlsym_error = {}", dlsym_error);
                    error = 1;
                    break;
                } else {
                    *tmp.p = p1;
                    AISDK_LOG_INFO("SNPELibWrapper load interface: {} succeed! ", tmp.name);
                }
            }

            if (error) {
                UnloadSnpe2CInterface();
            } else {
                *api = snpe2_provider_;
            }
        } else {
            const char* dlsym_error = dlerror();
            AISDK_LOG_ERROR("dlsym_error = {}", dlsym_error);
        }
    } else {
        *api = snpe2_provider_;
    }

    return snpe2_handle_ ? 0 : -1;
}

int SNPELibWrapper::LoadSnpe2PlatformCInterface(const char* snpe_soname, struct SnpePlatformCInterface* api) {
#define ConvertDlstruct(FUNNAME) \
    { (void**)&snpe2_platform_provider_.FUNNAME, #FUNNAME }

    struct Dlstruct {
        void** p;
        const char* name;
    };

    struct Dlstruct dllist[] = {
        /* SNPEUtil */
        ConvertDlstruct(Snpe_PlatformValidator_Create), ConvertDlstruct(Snpe_PlatformValidator_SetRuntime),
        ConvertDlstruct(Snpe_PlatformValidator_IsRuntimeAvailable), ConvertDlstruct(Snpe_PlatformValidator_Delete)};

    if (!snpe2_platform_handle_) {
        snpe2_platform_handle_ = dlopen(snpe_soname, RTLD_LAZY);
        if (snpe2_platform_handle_) {
            int error = 0;
            for (int i = 0; i < sizeof(dllist) / sizeof(Dlstruct); i++) {
                struct Dlstruct tmp = dllist[i];
                void* p1 = dlsym(snpe2_platform_handle_, tmp.name);
                if (NULL == p1) {
                    const char* dlsym_error = dlerror();
                    AISDK_LOG_ERROR("dlsym_error = {}", dlsym_error);
                    error = 1;
                    break;
                } else {
                    *tmp.p = p1;
                    AISDK_LOG_INFO("SNPELibWrapper load interface: {} succeed! ", tmp.name);
                }
            }

            if (error) {
                UnloadSnpe2PlatformCInterface();
            } else {
                *api = snpe2_platform_provider_;
            }
        } else {
            const char* dlsym_error = dlerror();
            AISDK_LOG_ERROR("dlsym_error = {}", dlsym_error);
        }
    } else {
        *api = snpe2_platform_provider_;
    }

    return snpe2_platform_handle_ ? 0 : -1;
}

int SNPELibWrapper::UnloadSnpe2CInterface() {
    if (snpe2_handle_) {
        dlclose(snpe2_handle_);
        snpe2_handle_ = NULL;
    }
    return 0;
}

int SNPELibWrapper::UnloadSnpe2PlatformCInterface() {
    if (snpe2_platform_handle_) {
        dlclose(snpe2_platform_handle_);
        snpe2_platform_handle_ = NULL;
    }
    return 0;
}

SNPELibWrapper::SNPELibWrapper(aisdk::xengine::PlatformEnv* env) {
    int res = LoadSnpe2CInterface("libSNPE.so", &snpe2_provider_);
    AISDK_LOG_ERROR("SNPELibWrapper init res: {}", res);
    if (0 != res && env && env->app_lib_path) {
        AISDK_LOG_ERROR("SNPELibWrapper try load fix path: {}", env->app_lib_path);
        std::string fixpath(env->app_lib_path);
        fixpath += "/libSNPE.so";
        res = LoadSnpe2CInterface(fixpath.c_str(), &snpe2_provider_);
        AISDK_LOG_ERROR("SNPELibWrapper init fix path res: {}", res);
    }

    if (0) {
        res = LoadSnpe2PlatformCInterface("libPlatformValidatorShared.so", &snpe2_platform_provider_);
        AISDK_LOG_ERROR("SNPEPlatformLibWrapper init res: {}", res);
    }
}

SNPELibWrapper::~SNPELibWrapper() {
    UnloadSnpe2CInterface();
    UnloadSnpe2PlatformCInterface();
}
