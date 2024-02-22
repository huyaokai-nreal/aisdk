#ifndef _MNN_MODEL_H_
#define _MNN_MODEL_H_

#include "aisdk/xengine/nr_mnn_header.h"
#include "aisdk/xengine/nrnn_model.h"

namespace Xengine {

class MNN_AIModel : public AIModel {
   public:
    MNN_AIModel(ModelConfig &config);
    virtual ~MNN_AIModel();
    bool IsShared() { return false; }

   private:
    std::unique_ptr<MNN::Interpreter> m_network;
};
}  // namespace Xengine

#endif
