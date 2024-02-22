#include "nrhal_backend.h"

#include <map>
#include <mutex>

#include "aisdk/xengine/backend/cpu/cpu_backend.h"

#if defined(HAVE_HAL_HEXAGON_DSP_OPS)
#include "aisdk/xengine/backend/dsp/hexagon_dsp_backend.h"
#endif

extern "C" {
SYM_EXPORT Xengine::Backend* _ZN2NR200TK7FUNC006E(Xengine::BackendConfig& config) {
    static std::map<Xengine::RuntimeType, Xengine::Backend*> gbackend;
    static std::once_flag gonce;
    std::call_once(gonce, [&]() {
        gbackend[Xengine::RuntimeType::CPU] = new Xengine::CpuBackend();
#if defined(HAVE_HAL_HEXAGON_DSP_OPS)
        gbackend[Xengine::RuntimeType::DSP] = new Xengine::HexagonDspBackend();
#endif
    });

    auto retend = gbackend.find(config.rt);
    if (retend != gbackend.end()) {
        return retend->second;
    }

    return nullptr;
}
}