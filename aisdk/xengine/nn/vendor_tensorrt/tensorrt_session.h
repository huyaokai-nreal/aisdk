#ifndef _TRT_SESSION_H_
#define _TRT_SESSION_H_

#include <iostream>
#include <sstream>

#include "nr_tensorrt_header.h"
#include "nrnn_model.h"
#include "nrnn_session.h"
#include "tensorrt_model.h"

namespace Xengine {

class TRT_Session : public Session {
   public:
    TRT_Session();
    virtual ~TRT_Session();

    // load_model，create_IoTensors
    Status Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig);

    Status Forword(ModelInfo &handle);

   private:
    void *ins_handle = nullptr;
};

}  // namespace Xengine

#endif
