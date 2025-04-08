
#if (defined(ANDROID) || defined(__ANDROID__))
#include <android/log.h>
#endif

#include <dlfcn.h>
#include <unwind.h>

#include <iomanip>
#include <iostream>

#include "aisdk/base/log.h"
#include "profiling.h"

__BEGIN_DECLS

struct BacktraceState {
    void **current;
    void **end;
};

static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context *context, void *arg) {
    BacktraceState *state = static_cast<BacktraceState *>(arg);
    uintptr_t pc = _Unwind_GetIP(context);
    if (pc) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        } else {
            *state->current++ = reinterpret_cast<void *>(pc);
        }
    }
    return _URC_NO_REASON;
}

static size_t captureBacktrace(void **buffer, size_t max) {
    BacktraceState state = {buffer, buffer + max};
    _Unwind_Backtrace(unwindCallback, &state);

    return state.current - buffer;
}

void SYM_HIDDEN dumpBacktrace(size_t max) {
#if (defined(ANDROID) || defined(__ANDROID__))
    __android_log_print(ANDROID_LOG_ERROR, "AISDK", "Custom dumping backtrace:");
#endif
    void *buffer[max];
    size_t count = captureBacktrace(buffer, max);
    for (size_t idx = 0; idx < count; ++idx) {
        const void *addr = buffer[idx];
        const char *symbol = "";

        Dl_info info;
        if (dladdr(addr, &info) && info.dli_sname) {
            symbol = info.dli_sname;
        }

#if (defined(ANDROID) || defined(__ANDROID__))
        __android_log_print(ANDROID_LOG_ERROR, "AISDK", "  #%2zu: %p  %s\n", idx, addr, symbol);
#endif
    }
}

void SYM_HIDDEN __attribute__((no_stack_protector)) __stack_chk_fail(void) {
#if (defined(ANDROID) || defined(__ANDROID__))
    __android_log_print(ANDROID_LOG_ERROR, "AISDK", "stack smashing detected at pc %p __stack_chk_fail at %p\n",
                        (void *)__builtin_return_address(0), (void *)__stack_chk_fail);
#endif
    dumpBacktrace(128);
    AISDK_LOG_ERROR("stack smashing detected at pc {} __stack_chk_fail at {}\n", (void *)__builtin_return_address(0),
                    (void *)__stack_chk_fail);
    void (*__stack_chk_fail_local)(void) = (void (*)(void))dlsym(RTLD_NEXT, "__stack_chk_fail");
    __stack_chk_fail_local();
}

void SYM_HIDDEN __attribute__((no_stack_protector)) abort(void) {
#if (defined(ANDROID) || defined(__ANDROID__))
    __android_log_print(ANDROID_LOG_ERROR, "AISDK", "abort at pc %p abort at %p\n", (void *)__builtin_return_address(0),
                        (void *)abort);
#endif

    dumpBacktrace(128);

    void __attribute__((noreturn)) (*abort_next)(void) =
        (void __attribute__((noreturn)) (*)(void))dlsym(RTLD_NEXT, "abort");
    abort_next();
}

__END_DECLS