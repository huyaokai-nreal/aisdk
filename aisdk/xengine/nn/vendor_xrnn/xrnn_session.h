#ifndef _XRNN_SESSION_H_
#define _XRNN_SESSION_H_

#include "aisdk/xengine/nrnn_session.h"
namespace aisdk::xengine {

class XRNN_Session : public Session {
   public:
    XRNN_Session();
    virtual ~XRNN_Session();

    // load_model，create_IoTensors
    Status Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig);

    // inference
    Status Forword(ModelInfo &handle);
};

}  // namespace aisdk::xengine
#endif