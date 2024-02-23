#include "nrhal_backend.h"

#include <map>
#include <mutex>

#include "aisdk/xengine/backend/cpu/cpu_backend.h"

#if defined(HAVE_HAL_HEXAGON_DSP_OPS)
#include "aisdk/xengine/backend/dsp/hexagon_dsp_backend.h"
#endif

extern "C" {
SYM_EXPORT aisdk::xengine::Backend* _ZN2NR200TK7FUNC006E(aisdk::xengine::BackendConfig& config) {
    static std::map<aisdk::xengine::RuntimeType, aisdk::xengine::Backend*> gbackend;
    static std::once_flag gonce;
    std::call_once(gonce, [&]() {
        gbackend[aisdk::xengine::RuntimeType::CPU] = new aisdk::xengine::CpuBackend();
#if defined(HAVE_HAL_HEXAGON_DSP_OPS)
        gbackend[aisdk::xengine::RuntimeType::DSP] = new aisdk::xengine::HexagonDspBackend();
#endif
    });

    auto retend = gbackend.find(config.rt);
    if (retend != gbackend.end()) {
        return retend->second;
    }

    return nullptr;
}
}