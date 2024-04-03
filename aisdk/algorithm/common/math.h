#pragma  once
#include<vector>
namespace aisdk::algorithm {
template<typename T>
std::vector<T> linspace(double start, double stop, int num_points, bool endpoint) {
    std::vector<T> result(num_points);
    double step = (stop - start) / static_cast<T>(endpoint ? num_points - 1 : num_points);

    for (int i = 0; i < num_points; ++i) {
        result[i] = start + i * step;
    }

    if (endpoint && num_points > 1) {
        // If endpoint is false, adjust the last point to be exactly 'stop'
        result[num_points - 1] = stop;
    }

    return result;
}
}