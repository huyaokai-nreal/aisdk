#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "models_load.h"

#include <cstring>

#include "aes.h"
#include "incbin.h"
#include "models.h"

// 自动绑定name变量和data变量
INCBIN(DEFAULT_PIPELINE_TAR_NAME, DEFAULT_PIPELINE_TAR_PATH);
static struct IncbinInfo g_incbininfo[] = {INCBIN_EXTERN_MY(DEFAULT_PIPELINE_TAR_NAME, DEFAULT_PIPELINE_TAR_KEY)};

extern "C" {
SYM_EXPORT bool _ZN2NR200TK7FUNC004E(std::string &name, struct IncbinInfo &info) {
    for (uint32_t i = 0; i < sizeof(g_incbininfo) / sizeof(struct IncbinInfo); i++) {
        if (0 == strcmp(g_incbininfo[i].name, (const char *)name.data())) {
            info = g_incbininfo[i];
            return true;
        }
    }
    return false;
}
}
