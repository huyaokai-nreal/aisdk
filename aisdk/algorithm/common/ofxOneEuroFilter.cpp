#include "ofxOneEuroFilter.h"

#include <memory>

namespace aisdk::algorithm {

void LowPassFilter::setAlpha(double alpha) {
    if (alpha <= 0.0 || alpha > 1.0) throw std::range_error("alpha should be in (0.0., 1.0]");
    a = alpha;
}

LowPassFilter::LowPassFilter(double alpha, double initval) {
    y = s = initval;
    setAlpha(alpha);
    initialized = false;
}

double LowPassFilter::filter(double value) {
    double result;
    if (initialized)
        result = a * value + (1.0 - a) * s;
    else {
        result = value;
        initialized = true;
    }
    y = value;
    s = result;
    return result;
}

double LowPassFilter::filterWithAlpha(double value, double alpha) {
    setAlpha(alpha);
    return filter(value);
}

OneEuroFilter::OneEuroFilter(double freq, double mincutoff, double beta_, double dcutoff) {
    setFrequency(freq);
    setMinCutoff(mincutoff);
    setBeta(beta_);
    setDerivateCutoff(dcutoff);
    x_ = std::make_unique<LowPassFilter>(alpha(mincutoff));
    dx_ = std::make_unique<LowPassFilter>(alpha(dcutoff));
}
void OneEuroFilter::reset() {
    x_->reset();
    dx_->reset();
    lasttime_ = UndefinedTime;
}
double OneEuroFilter::filter(double value, TimeStamp timestamp) {
    // update the sampling frequency based on timestamps
    if (lasttime_ != UndefinedTime && timestamp != UndefinedTime) freq_ = 1.0 / (timestamp - lasttime_);
    lasttime_ = timestamp;
    // estimate the current variation per second
    double dvalue = x_->hasLastRawValue() ? (value - x_->lastRawValue()) * freq_ : 0.0;  // FIXME: 0.0 or value?
    double edvalue = dx_->filterWithAlpha(dvalue, alpha(dcutoff_));
    // use it to update the cutoff frequency
    double cutoff = mincutoff_ + beta_ * fabs(edvalue);
    // filter the given value
    return x_->filterWithAlpha(value, alpha(cutoff));
}
}  // namespace aisdk::algorithm
