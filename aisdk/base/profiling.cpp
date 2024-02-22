#include "profiling.h"

namespace aisdk::base {

DebugProfiling& DebugProfiling::Get() {
    static DebugProfiling ins;
    return ins;
};

ProfilingOption& DebugProfiling::GetOpt() { return m_opt; }

void DebugProfiling::SetOpt(ProfilingOption& opt) { m_opt = opt; }

}  // namespace aisdk::base

extern "C" {
// 解决单例符号在.a中且被限制导出到so，引起在多个so中会多次实例化
SYM_EXPORT void _ZN2NR200TK7FUNC005E(aisdk::base::ProfilingOption& opt) {
    aisdk::base::DebugProfiling::Get().SetOpt(opt);
}
}