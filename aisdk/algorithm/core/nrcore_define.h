
#pragma once

#include <map>
#include <string>
#include <vector>

namespace aisdk::algorithm {

enum class Status {
    UNKNOWN = 0,
    SUCCESS = 1,
    FAILURE = 2,
    PIPELINE_INIT_FAILURE = 1000,
};

enum class CamType {
    UNKNOWN = 0,
    MONO = 1,
    BINO = 2,
};

struct CameraParams {
    std::map<std::string, std::vector<float>> m_params;
};

}  // namespace aisdk::algorithm
