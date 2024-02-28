#include "nrhal_capi_symbol.h"

namespace aisdk::xengine {

DlSymFuncs* GetXengineCapiStaticSymbol() {
    static DlSymFuncs ret;
    ret.m_createanalysistar = _ZN2NR200TK7FUNC007E;
    ret.m_destoryanalysistar = _ZN2NR200TK7FUNC008E;
    ret.m_createnetalgo = _ZN2NR200TK7FUNC002E;
    ret.m_destorynetalgo = _ZN2NR200TK7FUNC003E;
    ret.m_gethalbackend = _ZN2NR200TK7FUNC006E;
    ret.m_getplatform = _ZN2NR200TK7FUNC001E;
    ret.m_syncdebugprofiling = _ZN2NR200TK7FUNC005E;

    return &ret;
}

}  // namespace aisdk::xengine
