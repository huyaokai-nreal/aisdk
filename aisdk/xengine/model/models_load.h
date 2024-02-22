#ifndef _NRHAL_MODEL_LOAD_H_
#define _NRHAL_MODEL_LOAD_H_

#include "aisdk/xengine/nrhal_define.h"

extern "C" {

struct SYM_EXPORT IncbinInfo {
    // 这里不能有初值
    const char *name;
    const char *aeskey;
    const unsigned char *start;
    const unsigned char *end;
    uint32_t size;
};

typedef bool (*GetIncbinInfoFunc)(std::string &name, struct IncbinInfo &info);
SYM_EXPORT bool _ZN2NR200TK7FUNC004E(std::string &name, struct IncbinInfo &info);
}
#endif