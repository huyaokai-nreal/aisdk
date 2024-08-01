#include "time.h"

#include <absl/time/clock.h>

#include <utility>

#include "aisdk/base/log.h"

namespace aisdk::base {

std::string _CutParenthesesNTail(std::string &&prettyFuncon) {
    auto pos = prettyFuncon.find('(');
    if (pos != std::string::npos) {
        prettyFuncon.erase(prettyFuncon.begin() + pos, prettyFuncon.end());
    }

    return std::move(prettyFuncon);
}

TimerBase::TimerBase() { reset(); }

TimerBase::~TimerBase() {
    // do nothing
}

void TimerBase::reset() { last_reset_time = absl::Now(); }

uint64_t TimerBase::durationInUs() { return absl::ToInt64Microseconds(absl::Now() - last_reset_time); }

NaiveTimer::NaiveTimer(int line, const char *func) {
    name_ = strdup(_CutParenthesesNTail(func).c_str());
    line_ = line;
    tag_ = "null";
}

NaiveTimer::NaiveTimer(int line, const char *func, std::string _tag) {
    name_ = strdup(_CutParenthesesNTail(func).c_str());
    line_ = line;
    tag_ = std::move(_tag);
}
NaiveTimer::~NaiveTimer() {
    if (valid) {
        auto timeInUs = durationInUs();
        AISDK_LOG_WARN("[Name:{}],[Line:{}],[fun:{}],[id:{}],[cost:{:.3f}ms]", name_, line_, tag_.c_str(), id,
                       (double)timeInUs / 1000.0F);
    }
}

int NaiveTimer::BreakPoint(std::string &_tag2) {
    if (valid) {
        auto timeInUs = durationInUs();
        AISDK_LOG_WARN("[Name:{}],[Line:{}],[fun:{}],[bp:{}],[id:{}],[cost:{:.3f}ms]", name_, line_, tag_.c_str(),
                       _tag2.c_str(), id, (double)timeInUs / 1000.0F);
    }
    return 0;
}

uint64_t getTime2() {
    struct timespec cur_sys_t;
    clock_gettime(CLOCK_MONOTONIC, &cur_sys_t);
    uint64_t cur_sys_time = cur_sys_t.tv_sec * (uint64_t)1000000000 + cur_sys_t.tv_nsec;
    return cur_sys_time;
}

}  // namespace aisdk::base