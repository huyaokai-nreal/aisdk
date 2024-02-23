#ifndef _TRT_MODEL_H_
#define _TRT_MODEL_H_

#include "nr_tensorrt_header.h"
#include "nrnn_model.h"
#include "nrnn_session.h"

namespace aisdk::xengine {

class TRT_AIModel : public AIModel {
   public:
    TRT_AIModel(ModelConfig &config);
    virtual ~TRT_AIModel();
    bool IsShared() { return false; }

   public:
    ModelConfig m_config;
};
}  // namespace aisdk::xengine
#endif