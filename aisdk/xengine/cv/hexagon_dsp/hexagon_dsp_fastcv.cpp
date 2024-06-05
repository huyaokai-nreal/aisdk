
#include "hexagon_dsp_fastcv.h"

#include <dlfcn.h>

#include "aisdk/base/log.h"

namespace aisdk::xengine {

bool HexagonDspInterface::Init() {
    so_handle = dlopen("libxrealai_calculator_sdk.so", RTLD_LAZY);
    if (so_handle) {
        calculator_test = (xrealai_calculator_test)dlsym(so_handle, "xrealai_calculator_test");
        if (nullptr == calculator_test) {
            const char* dlsym_error = dlerror();
            AISDK_LOG_ERROR("dlsym_error = {}", dlsym_error);
        } else {
            return true;
        }
    } else {
        const char* dlopen_error = dlerror();
        AISDK_LOG_ERROR("dlopen_error = {}", dlopen_error);
    }
    return false;
}

bool HexagonDspInterface::DspSupport(int domain_id, bool is_unsignedpd) {
    if (calculator_test) {
        auto ret = calculator_test(0, domain_id, 32, is_unsignedpd);
        AISDK_LOG_ERROR("calculator_test ret = {}", ret);
    }
    return false;
}

}  // namespace aisdk::xengine