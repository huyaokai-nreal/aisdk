#ifndef _NRHAL_CAPI_SYM_H_
#define _NRHAL_CAPI_SYM_H_

#include "nrhal_common.h"
#include "nrhal_backend.h"
#include "nrhal_net.h"
#include "nr_model_mgr.h"
#include "aisdk/base/profiling.h"

namespace aisdk::xengine {

struct DlSymFuncs {
    GetPlatformStatusFunc m_getplatform = nullptr;
    CreateNetAlgoFunc m_createnetalgo = nullptr;
    DestoryNetAlgoFunc m_destorynetalgo = nullptr;
    CreateAnalysisTarFunc m_createanalysistar = nullptr;
    DestoryAnalysisTarFunc m_destoryanalysistar = nullptr;
    DebugProfilingOptionFunc m_syncdebugprofiling = nullptr;
    GetHalBackendFunc m_gethalbackend = nullptr;
};

// 只能在静态库中使用
DlSymFuncs* GetXengineCapiStaticSymbol();

}  // namespace aisdk::xengine


#endif