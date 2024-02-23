#include "aisdk/xengine/nrhal_common.h"
#include "xrnn_model.h"
#include "xrnn_session.h"

namespace aisdk::xengine {

XRNN_AIModel::XRNN_AIModel(ModelConfig &config) : AIModel() { (void)config.model_mem; }

XRNN_AIModel::~XRNN_AIModel() {}

XRNN_Session::XRNN_Session() : Session() {}
XRNN_Session::~XRNN_Session() {}

Status XRNN_Session::Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig) {
    (void)model.get();
    (void)Sconfig.batch;
    return Status::SUCCESS;
}

Status XRNN_Session::Forword(ModelInfo &handle) {
    (void)handle.handle;
    return Status::SUCCESS;
}

}  // namespace aisdk::xengine