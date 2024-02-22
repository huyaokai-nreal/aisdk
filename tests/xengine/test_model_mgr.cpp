#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "aisdk/base/log.h"
#include "aisdk/xengine/nr_model_mgr.h"
using namespace aisdk::base;

TEST_CASE("testing create netalgo") {
    std::string tar_name(NAME_TO_STRING(DEFAULT_PIPELINE_TAR_NAME));
    Xengine::AnalysisTar *tar_handle = _ZN2NR200TK7FUNC007E();
    bool ret = tar_handle->TarMem(tar_name.c_str());
    CHECK(!ret);
    auto& pipelineConfig = tar_handle->GetPipelineConfig();
    CHECK(!pipelineConfig.size());
    _ZN2NR200TK7FUNC008E(tar_handle);
}