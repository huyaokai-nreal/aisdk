#pragma once

#if defined(__linux__)
#include <cmath>
#include <ctime>
#endif
#include <stdexcept>

// -----------------------------------------------------------------
// Utilities

typedef double TimeStamp;  // in seconds

static const TimeStamp UndefinedTime = -1.0;

namespace aisdk::algorithm {

// -----------------------------------------------------------------

class LowPassFilter final {
   public:
    LowPassFilter(double alpha, double initval = 0.0);

    double filter(double value);

    double filterWithAlpha(double value, double alpha);

    bool hasLastRawValue(void) { return initialized; }

    double lastRawValue(void) { return y; }

   private:
    void setAlpha(double alpha);

    double y, a, s;
    bool initialized;
};

// -----------------------------------------------------------------

class OneEuroFilter final {
   public:
    OneEuroFilter(double freq, double mincutoff = 1.0, double beta_ = 0.0, double dcutoff = 1.0);

    double filter(double value, TimeStamp timestamp = UndefinedTime);

    ~OneEuroFilter(void);

   private:
    double freq;
    double mincutoff;
    double beta_;
    double dcutoff;
    LowPassFilter *x;
    LowPassFilter *dx;
    TimeStamp lasttime;

    double alpha(double cutoff) {
        double te = 1.0 / freq;
        double tau = 1.0 / (2 * M_PI * cutoff);
        return 1.0 / (1.0 + tau / te);
    }

    void setFrequency(double f) {
        if (f <= 0) throw std::range_error("freq should be >0");
        freq = f;
    }

    void setMinCutoff(double mc) {
        if (mc <= 0) throw std::range_error("mincutoff should be >0");
        mincutoff = mc;
    }

    void setBeta(double b) { beta_ = b; }

    void setDerivateCutoff(double dc) {
        if (dc <= 0) throw std::range_error("dcutoff should be >0");
        dcutoff = dc;
    }
};

}  // namespace aisdk::algorithm
