#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "aisdk/base/log.h"
#include "aisdk/xengine/nrhal_net.h"
using namespace aisdk::base;

TEST_CASE("testing create netalgo") {
    const char *algoname_version = "1.0.0";
    const char *netname = "AADSAEEEE";
    Xengine::ModelConfig model;
    Xengine::SessionConfig session;    
    Xengine::BaseNetAlgo* algo = _ZN2NR200TK7FUNC002E(algoname_version,netname,&model,&session);
    CHECK(!algo);
}