#pragma once
#include <absl/time/time.h>
#include <stdint.h>
#include <unistd.h>

#include <cstdint>
#include <string>

std::string _CutParenthesesNTail(std::string&& prettyFuncon);
namespace aisdk::base {

class TimerBase {
   public:
    TimerBase();
    ~TimerBase();
    TimerBase(const TimerBase&) = delete;
    TimerBase(const TimerBase&&) = delete;
    TimerBase& operator=(const TimerBase&) = delete;
    TimerBase& operator=(const TimerBase&&) = delete;

    // reset timer
    void reset();
    // get duration (us) from init or latest reset.
    uint64_t durationInUs();

   protected:
    absl::Time last_reset_time;
};

class NaiveTimer : TimerBase {
   public:
    NaiveTimer(int line, const char* func);
    NaiveTimer(int line, const char* func, std::string _tag);
    ~NaiveTimer();
    NaiveTimer(const NaiveTimer&) = delete;
    NaiveTimer(const NaiveTimer&&) = delete;
    NaiveTimer& operator=(const NaiveTimer&) = delete;
    NaiveTimer& operator=(const NaiveTimer&&) = delete;

   private:
    int line_;
    char* name_;
    std::string tag_;
    public:
    bool valid = true;
    uint64_t id = 0;
};
}  // namespace aisdk::base

#define TIMER_ONCE aisdk::base::NaiveTimer ___t(__LINE__, __PRETTY_FUNCTION__)
#define TIMER_ONCE_WITH_TAG(_Tag) aisdk::base::NaiveTimer ___t(__LINE__, __PRETTY_FUNCTION__, #_Tag)
#define TIMER_LOOP static aisdk::base::NaiveTimer ___t(__LINE__, __PRETTY_FUNCTION__)