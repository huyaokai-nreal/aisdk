#ifndef _XRNN_MODEL_H_
#define _XRNN_MODEL_H_

#include "aisdk/xengine/nrnn_model.h"

namespace aisdk::xengine {

class XRNN_AIModel : public AIModel {
   public:
    XRNN_AIModel(ModelConfig &config);
    virtual ~XRNN_AIModel();
    bool IsShared() { return false; }
};
}  // namespace aisdk::xengine

#endif
