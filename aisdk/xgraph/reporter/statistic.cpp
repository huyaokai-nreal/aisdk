#include "statistic.h"

#include <cmath>

#include "mediapipe/framework/calculator.pb.h"
#include "mediapipe/framework/calculator_profile.pb.h"

namespace xgraph::reporter {
double calculate_95th_percentile(const std::vector<double>& data) {
    if(data.empty()) {
      return -1;
}
    size_t n = data.size();
    double k = n * 0.95; // 计算95%的位置
    auto new_data = data;

    std::sort(new_data.begin(), new_data.end()); // 排序向量

    if (k == static_cast<int>(k)) { // 如果位置是整数
        // 取该位置的值
        return new_data[static_cast<size_t>(k) - 1];
    } else {
        // 否则，取两个最近的值并插值
        size_t lower_index = static_cast<size_t>(k);
        size_t upper_index = lower_index + 1;
        double lower_value = new_data[lower_index - 1];
        double upper_value = new_data[upper_index - 1];
        double fraction = k - static_cast<double>(lower_index);

        return lower_value + fraction * (upper_value - lower_value);
    }
}
// Pushes a single value into the statistics, updating mean and stddev.
void Statistic::Push(double x) {
  ++counter_;
  if (x > max_){
    max_ = x;
  }

  if (counter_ == 1) {
    mean_ = x;
    ssd_ = 0.0;
    total_impl_ = x;
  } else {
    // Implementing Welford’s algorithm for computing variance.
    auto old_mean = mean_;
    mean_ = mean_ + (x - mean_) / counter_;
    ssd_ = ssd_ + (x - mean_) * (x - old_mean);
    total_impl_ += x;
  }
  data_.push_back(x);
}

// Returns the number of data points used to calculator the mean and
// stddev.
int Statistic::data_count() const { return counter_; }

// Returns the mean of the data pushed into this statistic.
double Statistic::mean() const { return (counter_ > 0) ? mean_ : 0.0; }

// Returns the variance of the data pushed into this statistic.
double Statistic::variance() const {
  return ((counter_ > 1) ? ssd_ / (counter_ - 1) : 0.0);
}

// Returns the standard deviation of the data pushed into this statistic.
double Statistic::stddev() const { return std::sqrt(variance()); }

double Statistic::total() const { return total_impl_; }
double Statistic::max() const { return max_; }
double Statistic::percentile_95() const {return calculate_95th_percentile(data_);}

}  // namespace xgraph::reporter
