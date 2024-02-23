#include "nrhal_common.h"

#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <csetjmp>
#include <sstream>
#include <string>

#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"

#if defined(HAVE_HAL_SNPE)
#include "nr_snpe_header.h"
#endif

#if defined(HAVE_HAL_TENSORRT)
#include "dlutil.h"
#include "nr_tensorrt_header.h"
#endif

#if __APPLE__
#include "TargetConditionals.h"
#if __aarch64__
#include <sys/sysctl.h>
#endif
#endif  // __APPLE__

void PrintfHalModelConfig(aisdk::xengine::ModelConfig& info) {
    AISDK_LOG_TRACE("[HalModelConfig] model_path={} model_mem={} model_size={} vendor_type={}", info.model_path.c_str(),
                    static_cast<const void*>(info.model_mem), info.model_size, (int)info.vendor_type);
}

void PrintfHalSessionConfig(aisdk::xengine::SessionConfig& info) {
    AISDK_LOG_TRACE("[HalSessionConfig] batch={} precision={} threads_num={} runtime_order.size={}", (int)info.batch,
                    (int)info.precision, (int)info.threads_num, (int)info.runtime_order.size());
    for (auto& iter : info.runtime_order) {
        AISDK_LOG_TRACE("[HalSessionConfig] runtime_order={}", (int)iter);
    }
    for (auto& iter : info.customize_ioname.input_layername) {
        AISDK_LOG_TRACE("[HalSessionConfig] input_layername={}", iter.c_str());
    }
    for (auto& iter : info.customize_ioname.input_tensorname) {
        AISDK_LOG_TRACE("[HalSessionConfig] input_tensorname={}", iter.c_str());
    }
    for (auto& iter : info.customize_ioname.output_layername) {
        AISDK_LOG_TRACE("[HalSessionConfig] output_layername={}", iter.c_str());
    }
    for (auto& iter : info.customize_ioname.output_tensorname) {
        AISDK_LOG_TRACE("[HalSessionConfig] output_tensorname={}", iter.c_str());
    }
}

// 像numpy格式化打印tensor
std::string LoopDumpDims(aisdk::xengine::Tensor& info, uint32_t dims_index, float* addr) {
    if (dims_index < info.m_dims.size() - 1) {
        std::string tmp = "[";

        uint32_t offset = 1;
        for (uint32_t i = dims_index + 1; i < info.m_dims.size(); i++) {
            offset *= info.m_dims[i];
        }

        uint32_t loop_n = info.m_dims[dims_index];
        for (uint32_t i = 0; i < loop_n; i++) {
            tmp += LoopDumpDims(info, dims_index + 1, addr + i * offset);
            if (i < loop_n - 1) {
                tmp += ",\n";
            }
        }
        tmp += "]\n\n";
        return tmp;
    } else {
        std::string tmp = "[";
        uint32_t loop_n = info.m_dims[dims_index];
        for (uint32_t i = 0; i < loop_n; i++) {
            tmp += std::to_string(addr[i]);
            if (i < loop_n - 1) {
                tmp += ",";
            }
        }
        tmp += "]";
        return tmp;
    }
}

std::string DumpHalTensorMem(aisdk::xengine::Tensor& info) { return LoopDumpDims(info, 0, (float*)info.m_viraddr); }

const char* Dimtype2Str(aisdk::xengine::TensorFormat& dimtype) {
    const char* str = "unknwon";
    if (dimtype == aisdk::xengine::TensorFormat::NCHW) {
        str = "NCHW";
    } else if (dimtype == aisdk::xengine::TensorFormat::NHWC) {
        str = "NHWC";
    } else if (dimtype == aisdk::xengine::TensorFormat::CHW) {
        str = "CHW";
    } else if (dimtype == aisdk::xengine::TensorFormat::HWC) {
        str = "HWC";
    } else if (dimtype == aisdk::xengine::TensorFormat::NHW) {
        str = "NHW";
    } else if (dimtype == aisdk::xengine::TensorFormat::HW) {
        str = "HW";
    } else if (dimtype == aisdk::xengine::TensorFormat::NW) {
        str = "NW";
    } else if (dimtype == aisdk::xengine::TensorFormat::W) {
        str = "W";
    } else if (dimtype == aisdk::xengine::TensorFormat::NCDHW) {
        str = "NCDHW";
    } else if (dimtype == aisdk::xengine::TensorFormat::NDHWC) {
        str = "NDHWC";
    } else if (dimtype == aisdk::xengine::TensorFormat::CDHW) {
        str = "CDHW";
    } else if (dimtype == aisdk::xengine::TensorFormat::DHWC) {
        str = "DHWC";
    }

    return str;
}
void PrintfHalTensor(aisdk::xengine::Tensor& info) {
    // clang-format off
    AISDK_LOG_TRACE(
        "[Tensor] m_name={:s} m_rank={:d} dims.size={:d} m_dimtype={:s} m_elementype={:d} m_elementbyte={:d} m_elementsize={:d} m_viraddr={}",
        info.m_name.c_str(), (int)info.m_rank, (int)info.m_dims.size(), Dimtype2Str(info.m_dimtype), (int)info.m_elementype,
        (int)info.m_elementbyte, (int)info.m_elementsize, static_cast<void*>(info.m_viraddr));
    // clang-format on
    char tmpbuffer[128] = {0};
    for (uint32_t i = 0; i < info.m_rank; i++) {
        snprintf(tmpbuffer + strlen(tmpbuffer), sizeof(tmpbuffer) - 1, "%d,", (int)info.m_dims[i]);
    }
    AISDK_LOG_TRACE("[Tensor] dim={:s}", tmpbuffer);
}

void PrintfHalIoTensors(aisdk::xengine::IoTensors& info) {
    // clang-format off
    AISDK_LOG_TRACE("[HalIoTensors] batch={:d} ori_batch={:d} m_multishape_num={:d} m_packed_bybatch={:d} tensors.size={:d}",
                    (int)info.m_batch, (int)info.m_ori_batch, (int)info.m_multishape_num, (int)info.m_packed_bybatch,
                    (int)info.m_tensors.size());
    // clang-format on
    for (auto& iter : info.m_tensors) {
        PrintfHalTensor(iter);
    }
}

#if defined(__APPLE__) && defined(__aarch64__)

static uint32_t get_sys_info_by_name(const char* type_specifier) {
    size_t size = 0;
    uint32_t result = 0;
    if (sysctlbyname(type_specifier, NULL, &size, NULL, 0) != 0) {
        AISDK_LOG_TRACE("sysctlbyname(\"%s\") failed", type_specifier);
    } else if (size == sizeof(uint32_t)) {
        sysctlbyname(type_specifier, &result, &size, NULL, 0);
        AISDK_LOG_TRACE("%s: %u , size = %lu", type_specifier, result, size);
    } else {
        AISDK_LOG_TRACE("sysctl does not support non-integer lookup for (\"%s\")", type_specifier);
    }
    return result;
}

#endif  // iOS

void supportUpdata(aisdk::xengine::PlatformStatus& status) {
#if defined(__ANDROID__) || defined(__linux__)
    FILE* fp = fopen("/proc/cpuinfo", "rb");
    if (!fp) {
        return;
    }

    char buffer[1024];
    std::string featu;
    while (!feof(fp)) {
        char* str = fgets(buffer, 1024, fp);
        if (!str) {
            break;
        }

        if (memcmp(buffer, "Features", 8) == 0) {
            featu = buffer;
            if (featu.find("asimddp") != std::string::npos) {
                status.is_dot_support = true;
            }
            if (featu.find("asimdhp") != std::string::npos) {
                status.is_fp16_support = true;
            }
        }
    }
    fclose(fp);
#elif defined(__APPLE__) && defined(__aarch64__) && !defined(__IOS__)
// arm64-osx
#ifndef CPUFAMILY_AARCH64_FIRESTORM_ICESTORM
#define CPUFAMILY_AARCH64_FIRESTORM_ICESTORM 0x1b588bb3
#endif
    const uint32_t cpu_family = get_sys_info_by_name("hw.cpufamily");
    status.is_fp16_support = cpu_family == CPUFAMILY_AARCH64_FIRESTORM_ICESTORM;
    status.is_dot_support = cpu_family == CPUFAMILY_AARCH64_FIRESTORM_ICESTORM;
#endif
}

#if defined(HAVE_HAL_TENSORRT)
static TrtExecApi g_trtexec_api;

TrtExecApi GetTrtExecApi() { return g_trtexec_api; }

bool DlopenTrtExecDLL(NrHal::PlatformStatus& status) {
    static NrUtils::LibraryHandle g_trtexec_handle = NULL;
    if (!g_trtexec_handle) {
        std::string trtexecdll = "trtexec.dll";
        g_trtexec_handle = NrUtils::LibraryOpen(trtexecdll);
        if (g_trtexec_handle) {
            auto g_getcudadevice = (_getcudadevice)NrUtils::LibraryGetProcAddr(g_trtexec_handle, "getcudadevice");
            AISDK_LOG_TRACE("g_getcudadevice=%p", g_getcudadevice);
            auto g_maketrtexec = (_maketrtexec)NrUtils::LibraryGetProcAddr(g_trtexec_handle, "maketrtexec");
            AISDK_LOG_TRACE("g_maketrtexec=%p", g_maketrtexec);
            auto g_loadmemmodel = (_loadmemmodel)NrUtils::LibraryGetProcAddr(g_trtexec_handle, "loadmemmodel");
            AISDK_LOG_TRACE("g_loadmemmodel=%p", g_loadmemmodel);
            auto g_querytrttensor = (_querytrttensor)NrUtils::LibraryGetProcAddr(g_trtexec_handle, "querytrttensor");
            AISDK_LOG_TRACE("g_querytrttensor=%p", g_querytrttensor);
            auto g_runinfer = (_runinfer)NrUtils::LibraryGetProcAddr(g_trtexec_handle, "runinfer");
            AISDK_LOG_TRACE("g_runinfer=%p", g_runinfer);
            auto g_deltrtexec = (_deltrtexec)NrUtils::LibraryGetProcAddr(g_trtexec_handle, "deltrtexec");
            AISDK_LOG_TRACE("g_deltrtexec=%p", g_deltrtexec);
            if (g_getcudadevice && g_maketrtexec && g_loadmemmodel && g_querytrttensor && g_runinfer && g_deltrtexec) {
                g_trtexec_api.g_getcudadevice = g_getcudadevice;
                g_trtexec_api.g_maketrtexec = g_maketrtexec;
                g_trtexec_api.g_loadmemmodel = g_loadmemmodel;
                g_trtexec_api.g_querytrttensor = g_querytrttensor;
                g_trtexec_api.g_runinfer = g_runinfer;
                g_trtexec_api.g_deltrtexec = g_deltrtexec;

                WrapCudaDevice info;
                g_getcudadevice(&info);
                status.gpu_device_count = info.device_count;
                for (int i = 0; i < info.device_count; i++) {
                    AISDK_LOG_TRACE("gpu_device_name=%s", info.device_name[i]);
                    status.gpu_device_name.push_back(std::string(info.device_name[i]));
                }
            } else {
                const char* dlsym_error = NrUtils::LibraryGetProcAddrError("241").c_str();
                AISDK_LOG_TRACE("%s", dlsym_error);
                NrUtils::LibraryClose(g_trtexec_handle);
                g_trtexec_handle = NULL;
                return false;
            }
        } else {
            const char* dlsym_error = NrUtils::LibraryOpenError(trtexecdll).c_str();
            AISDK_LOG_TRACE("%s", dlsym_error);
            return false;
        }
    }

    return true;
}

#endif

#if defined(HAVE_HAL_SNPE)

bool checkSnapDragonAdspPath() {
    auto result_1 = int(!access("/system/lib/rfsa/adsp", F_OK));
    auto result_2 = int(!access("/system/vendor/lib/rfsa/adsp", F_OK));
    auto result_3 = int(!access("/dsp", F_OK));
    auto result = result_1 + result_2 + result_3;
    return (result > 0) ? true : false;
}

static jmp_buf kansnpe;
static sighandler_t old_handle;
void SignalHandler(int sig) {
    (void)sig;
    signal(SIGSEGV, old_handle);
    longjmp(kansnpe, 1);
}

#if SNPE_VERSION > 2000
#include "aisdk/xengine/nn/vendor_snpe2/snpe_lib_wrapper.h"
bool checkHexagonDSP() {
    SnpeCInterface temp;
    AISDK_LOG_TRACE("checkHexagonDSP");
    if (0 != SNPELibWrapper::getInstance().getSnpe2CInterface(&temp)) {
        AISDK_LOG_TRACE("getProviderfailed!");
        return false;
    }
    bool res = temp.Snpe_Util_IsRuntimeAvailable(Snpe_Runtime_t::SNPE_RUNTIME_DSP);
    AISDK_LOG_TRACE("Snpe_Util_IsRuntimeAvailable: {}", res);
    if (!res) {
        AISDK_LOG_ERROR("checkHexagonDSP failed: {}", temp.Snpe_ErrorCode_GetLastErrorString());
    }
    return res;
}

bool checkHexagonUnsignedPDDSP() {
    SnpeCInterface temp;
    AISDK_LOG_TRACE("checkHexagonUnsignedPDDSP");
    if (0 != SNPELibWrapper::getInstance().getSnpe2CInterface(&temp)) {
        AISDK_LOG_TRACE("getProviderfailed!");
        return false;
    }
    bool res = temp.Snpe_Util_IsRuntimeAvailableCheckOption(
        Snpe_Runtime_t::SNPE_RUNTIME_DSP, Snpe_RuntimeCheckOption_t::SNPE_RUNTIME_CHECK_OPTION_UNSIGNEDPD_CHECK);
    AISDK_LOG_TRACE("Snpe_Util_IsRuntimeAvailableCheckOption: {}", res);
    if (!res) {
        AISDK_LOG_ERROR("checkHexagonUnsignedPDDSP failed: {}", temp.Snpe_ErrorCode_GetLastErrorString());
    }
    return res;
}

#else
bool checkHexagonDSP() { return zdl::SNPE::SNPEFactory::isRuntimeAvailable(zdl::DlSystem::Runtime_t::DSP); }

bool checkHexagonUnsignedPDDSP() {
    return zdl::SNPE::SNPEFactory::isRuntimeAvailable(zdl::DlSystem::Runtime_t::DSP,
                                                      zdl::DlSystem::RuntimeCheckOption_t::UNSIGNEDPD_CHECK);
}
#endif

void checkSnapdragonSoc(aisdk::xengine::PlatformStatus& status) {
    {
        char buffer[128] = {0};
        FILE* piper = popen("cat /sys/devices/soc0/chip_name", "r");
        if (piper) {
            auto ret = fgets(buffer, sizeof(buffer), piper);
            (void)ret;
            pclose(piper);
            if (buffer[0] != '\0') {
                if (0 == strncmp(buffer, "SDM855", 6)) {
                    status.is_snapdragon_855 = true;
                }
                if (0 == strncmp(buffer, "SM_SAIPAN", 9)) {
                    status.is_snapdragon_855 = true;
                }
                if (0 == strncmp(buffer, "SM_WAIPIO", 9)) {
                    status.is_snapdragon_8Gen1 = true;
                }
                if (0 == strncmp(buffer, "SM_PALIMA", 9)) {
                    status.is_snapdragon_8Gen1 = true;  // plus
                }
            }
        }
    }

    {
        char buffer[128] = {0};
        FILE* piper = popen("cat /sys/devices/soc0/soc_id", "r");
        if (piper) {
            auto ret = fgets(buffer, sizeof(buffer), piper);
            (void)ret;
            pclose(piper);
            if (buffer[0] != '\0') {
                if (0 == strncmp(buffer, "339", 3)) {
                    status.is_snapdragon_855 = true;
                }
                if (0 == strncmp(buffer, "440", 3)) {
                    status.is_snapdragon_855 = true;
                }
                if (0 == strncmp(buffer, "457", 3)) {
                    status.is_snapdragon_8Gen1 = true;
                }
                if (0 == strncmp(buffer, "530", 3)) {
                    status.is_snapdragon_8Gen1 = true;  // plus
                }
                if (0 == strncmp(buffer, "540", 3)) {
                    status.is_snapdragon_8Gen1 = true;  // plus
                }
            }
        }
    }

    {
        char buffer[128] = {0};
        FILE* piper = popen("cat /proc/cpuinfo | grep Hardware", "r");
        if (piper) {
            auto ret = fgets(buffer, sizeof(buffer), piper);
            (void)ret;
            pclose(piper);
            if (buffer[0] != '\0') {
                char* pch = nullptr;
                if ((pch = strstr(buffer, "SDM855"))) {
                    status.is_snapdragon_855 = true;
                }
                if ((pch = strstr(buffer, "SM8150"))) {
                    status.is_snapdragon_855 = true;
                }
                if ((pch = strstr(buffer, "SM8450"))) {
                    status.is_snapdragon_8Gen1 = true;
                }
                if ((pch = strstr(buffer, "SM8475"))) {
                    status.is_snapdragon_8Gen1 = true;  // plus
                }
            }
        }
    }
}
#endif

bool CheckEngineSingleBatch(aisdk::xengine::VendorType& vendor) {
    (void)vendor;
#if defined(HAVE_HAL_SNPE)
#if SNPE_VERSION == SNPE_1660
    if (vendor == aisdk::xengine::VendorType::SNPE) {
        aisdk::xengine::PlatformStatus st = _ZN2NR200TK7FUNC001E();
        if (st.is_snapdragon_8Gen1) {
            return true;
        }
    }
#endif
#endif
    return false;
}

// static std::map<std::string, NrUtils::TRecord> hal_debug_timerecord;
// static std::string hal_debug_timerecord_file;
// void HalInitDebugTRecord() {
//     auto& prof = NrUtils::DebugProfiling::Get().GetOpt();
//     if (prof.pipeline_node_time_statistics) {
//         time_t time_s;
//         time(&time_s);
//         char timestamp[64] = {0};
//         strftime(timestamp, sizeof(timestamp), "%Y-%m-%d-%H-%M-%S", localtime(&time_s));
//         std::string local_record_rootpath =
//             prof.local_data_record_rootpath + "/developer_test_" + std::string(timestamp);
//         NrUtils::createFolderIfNotExist(local_record_rootpath);
//         hal_debug_timerecord_file = local_record_rootpath + "/time_statistics_hal.txt";
//         AISDK_LOG_TRACE("hal_debug_timerecord_file=%s", hal_debug_timerecord_file.c_str());
//     }
// }

// NrUtils::TRecord* HalGetDebugTRecord(const char* time_name) {
//     NrUtils::TRecord* ret = nullptr;
//     std::string type = time_name;
//     auto iter = hal_debug_timerecord.find(type);
//     if (iter == hal_debug_timerecord.end()) {
//         auto pa = hal_debug_timerecord.insert(std::make_pair(type, NrUtils::TRecord()));
//         ret = &pa.first->second;
//     } else {
//         ret = &iter->second;
//     }

//     return ret;
// }

// void HalShowDebugTRecord(int interval) {
//     static uint64_t total = 0;
//     auto& prof = NrUtils::DebugProfiling::Get().GetOpt();
//     if (prof.pipeline_node_time_statistics) {
//         if (total % interval == 0) {
//             std::string write_string;
//             for (auto& [k, v] : hal_debug_timerecord) {
//                 write_string += NrUtils::string_sprintf("TRecord name=%s,count=%llu,average_time_us=,%llu,(us)\n",
//                                                         k.c_str(), v.count, (uint64_t)v.average_time_us);
//             }

//             NrUtils::AppendWriteToFile(hal_debug_timerecord_file, write_string);
//         }
//         total++;
//     }
// }

extern "C" {

SYM_EXPORT aisdk::xengine::PlatformStatus* _ZN2NR200TK7FUNC001E() {
    static aisdk::xengine::PlatformStatus ret;
    static std::once_flag oc;
    std::call_once(oc, [&]() {
        supportUpdata(ret);
#if defined(HAVE_HAL_SNPE)
#if (defined(ANDROID) || defined(__ANDROID__))
        // if (checkSnapDragonAdspPath()) {
        {
            checkSnapdragonSoc(ret);

            // 以下执行可能会崩溃在libsnpe.so中，代码规避下：
            if (setjmp(kansnpe) == 0) {
                ret.is_hexagon_dsp = checkHexagonDSP();
#if SNPE_VERSION > 2000
                ret.is_hexagon_unsignedPD_dsp = checkHexagonUnsignedPDDSP();
#else
                if (false == ret.is_hexagon_dsp) {
                    ret.is_hexagon_unsignedPD_dsp = checkHexagonUnsignedPDDSP();
                }
#endif
            } else {
                ret.is_hexagon_dsp = false;
                ret.is_hexagon_unsignedPD_dsp = false;
            }

            if (ret.is_hexagon_dsp || ret.is_hexagon_unsignedPD_dsp) {
                ret.is_snpe_support = true;
            }
        }
#elif defined(__linux__)
        ret.is_snpe_support = true;
#endif

#endif
        auto& profcnf = aisdk::base::DebugProfiling::Get().GetOpt();
        if (profcnf.loglevel_trace) {
            aisdk::base::Logger::GetInstance()->SetLogAllLevel(true);
        }
#if defined(HAVE_HAL_TENSORRT)
        DlopenTrtExecDLL(ret);
#endif

#if defined(HAVE_HAL_ARTOSYN)
        ret.is_artosyn_support = true;
#endif
        // HalInitDebugTRecord();
    });

    return &ret;
}
}