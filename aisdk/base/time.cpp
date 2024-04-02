#include "aisdk/base/time.h"

#include "aisdk/base/log.h"

namespace aisdk::base {

std::string _CutParenthesesNTail(std::string &&prettyFuncon) {
    auto pos = prettyFuncon.find('(');
    if (pos != std::string::npos) prettyFuncon.erase(prettyFuncon.begin() + pos, prettyFuncon.end());

    return std::move(prettyFuncon);
}

TimerBase::TimerBase() { reset(); }

TimerBase::~TimerBase() {
    // do nothing
}

void TimerBase::reset() {
    struct timeval Current;
    gettimeofday(&Current, nullptr);
    last_reset_time_ = Current.tv_sec * 1000000 + Current.tv_usec;
}

uint64_t TimerBase::durationInUs() {
    struct timeval Current;
    gettimeofday(&Current, nullptr);
    auto lastTime = Current.tv_sec * 1000000 + Current.tv_usec;

    return lastTime - last_reset_time_;
}

NaiveTimer::NaiveTimer(int line, const char *func) : TimerBase() {
    name_ = strdup(_CutParenthesesNTail(func).c_str());
    line_ = line;
    tag_ = "null";
}

NaiveTimer::NaiveTimer(int line, const char *func, std::string _tag) : TimerBase() {
    name_ = strdup(_CutParenthesesNTail(func).c_str());
    line_ = line;
    tag_ = _tag;
}
NaiveTimer::~NaiveTimer() {
    auto timeInUs = durationInUs();
    AISDK_LOG_INFO("[Name:{}],[Line:{}],[fun:{}],[cost:{:.3f}ms]", name_, line_, tag_.c_str(),
                   (float)timeInUs / 1000.0f);
}

}  // namespace aisdk::base