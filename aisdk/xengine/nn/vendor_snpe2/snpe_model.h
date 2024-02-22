#ifndef _SNPE_MODEL_H_
#define _SNPE_MODEL_H_

#include "aisdk/xengine/nr_snpe_header.h"
#include "aisdk/xengine/nrnn_model.h"
namespace Xengine {

class SNPE_AIModel : public AIModel {
   public:
    SNPE_AIModel(ModelConfig &config);
    virtual ~SNPE_AIModel();
    bool IsShared() { return true; }

   public:
    // Since update to snpe2 interface, save a temporary config here, model init moved to snpe session
    ModelConfig m_config;
};
}  // namespace Xengine

#endif