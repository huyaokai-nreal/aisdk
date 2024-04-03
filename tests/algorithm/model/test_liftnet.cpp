#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "opencv2/opencv.hpp"
#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"
#include "aisdk/xengine/nr_model_mgr.h"
#include "aisdk/xengine/nrhal_net.h"
#include "aisdk/algorithm/model/hand_detect.h"

TEST_CASE("testing create netalgo") {
    aisdk::base::Logger::GetInstance()->SetLogAllLevel(true);  // 打开日志
}