#pragma once

#include <memory>
#if defined(__linux__)
#include <cmath>
#include <ctime>
#endif
#include <stdexcept>

// -----------------------------------------------------------------
// Utilities

using TimeStamp = double;  // in seconds

static const TimeStamp UndefinedTime = -1.0;

namespace aisdk::algorithm {

// -----------------------------------------------------------------

class LowPassFilter final {
   public:
    // Constructs a LowPassFilter with a given alpha and initial value.
    // @param alpha: The smoothing factor.
    // @param initval: The initial value of the filter (default is 0.0).
    explicit LowPassFilter(double alpha, double initval = 0.0);

    // Filters a new value through the low-pass filter.
    // @param value: The new value to be filtered.
    // @return: The filtered value.
    double filter(double value);

    // Filters a new value with a specified alpha.
    // @param value: The new value to be filtered.
    // @param alpha: The smoothing factor to be used for this filtering.
    // @return: The filtered value.
    double filterWithAlpha(double value, double alpha);

    // Checks if the filter has a last raw value.
    // @return: True if the filter has a last raw value, otherwise false.
    bool hasLastRawValue(void) const { return initialized; }

    // Returns the last raw value filtered by the filter.
    // @return: The last raw value.
    double lastRawValue(void) const { return y; }
    
    // Resets the filter to its initial state.
    void reset(){
        initialized = false;
    }

   private:
    // Sets a new alpha value for the filter.
    // @param alpha: The new smoothing factor.
    void setAlpha(double alpha);

    double y, a, s;  // y: filtered value, a: alpha, s: state variable for initialization control
    bool initialized = false;  // Flag to check if the filter is initialized
};

// -----------------------------------------------------------------

class OneEuroFilter final {
   public:
    // Constructs an OneEuroFilter with specified parameters.
    // @param freq: The frequency of the signal.
    // @param mincutoff: The minimum cutoff frequency (default is 1.0).
    // @param beta_: The cutoff frequency for the derivative (default is 0.0).
    // @param dcutoff: The cutoff frequency for the derivative of the signal (default is 1.0).
    explicit OneEuroFilter(double freq, double mincutoff = 1.0, double beta_ = 0.0, double dcutoff = 1.0);

    // Filters a new value with an optional timestamp.
    // @param value: The new value to be filtered.
    // @param timestamp: The timestamp of the new value (default is UndefinedTime).
    // @return: The filtered value.
    double filter(double value, TimeStamp timestamp = UndefinedTime);
    
    // Resets the filter to its initial state.
    void reset();
   private:
    double freq_;  // Frequency of the signal
    double mincutoff_;  // Minimum cutoff frequency
    double beta_;  // Cutoff frequency for the derivative
    double dcutoff_;  // Cutoff frequency for the derivative of the signal
    std::unique_ptr<LowPassFilter> x_;  // Pointer to the LowPassFilter for the signal itself
    std::unique_ptr<LowPassFilter> dx_;  // Pointer to the LowPassFilter for the derivative of the signal
    TimeStamp lasttime_;  // Timestamp of the last filtered value

    // Calculates the alpha value based on the cutoff frequency.
    // @param cutoff: The desired cutoff frequency.
    // @return: The calculated alpha value.
    double alpha(double cutoff) const {
        double te = 1.0 / freq_;  // Time constant for the exponential decay of the filter state
        double tau = 1.0 / (2 * M_PI * cutoff);  // Time constant for the cutoff frequency of the filter state
        return 1.0 / (1.0 + tau / te);  // Alpha value for the exponential decay filter state update
    }

    // Sets the frequency of the signal.
    // @param f: The new frequency value. Must be > 0.
    void setFrequency(double f) {
        if (f <= 0) { throw std::range_error("freq should be >0");  // Error handling for invalid frequency input
}
        freq_ = f;  // Update the frequency attribute with the new value
    }

    // Sets the minimum cutoff frequency. Must be > 0.
    // @param mc: The new minimum cutoff frequency value. Must be > 0.
    void setMinCutoff(double mc) {
        if (mc <= 0) { throw std::range_error("mincutoff should be >0");  // Error handling for invalid minimum cutoff input
}
        mincutoff_ = mc;  // Update the minimum cutoff attribute with the new value
    }

    // Sets the beta value for the cutoff frequency of the derivative. Must be > 0.
    // @param b: The new beta value. Must be > 0. If not provided, it defaults to 0.0.
    void setBeta(double b) { beta_ = b; }  // Update the beta attribute with the new value or default to 0.0 if not provided

    // Sets the cutoff frequency for the derivative of the signal. Must be > 0.
    // @param dc: The new cutoff frequency for the derivative of the signal value. Must be > 0. If not provided, it defaults to 1.0.
    void setDerivateCutoff(double dc) {
        if (dc <= 0) { throw std::range_error("dcutoff should be >0");
}
        dcutoff_ = dc;
    }
};

}  // namespace aisdk::algorithm
