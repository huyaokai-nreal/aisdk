/*
 * @Author: jszhang jszhang@nreal.ai
 * @Date: 2023-04-10 05:58:56
 * @LastEditors: jszhang jszhang@nreal.ai
 * @LastEditTime: 2023-04-10 06:13:37
 * @FilePath: /nreal_hand_demo_android/src/core/hal/nn/vendor_snpe/snpe_session.h
 */
#ifndef _SNPE_SESSION_H_
#define _SNPE_SESSION_H_

#include "aisdk/xengine/nr_snpe_header.h"
#include "aisdk/xengine/nrnn_model.h"
#include "aisdk/xengine/nrnn_session.h"
#include "snpe_wrapper.h"
namespace Xengine {

#define BUFFERTYPE_USER (1)
// #define BUFFERTYPE_ITENSER (2) // for test

class SNPE_Session : public Session {
   public:
    SNPE_Session();
    virtual ~SNPE_Session();

    // load_model，create_IoTensors
    Status Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig);

    // inference
    Status Forword(ModelInfo &handle);

   protected:
    std::unique_ptr<SNPEWrapper> mSnpeWrapper;
};

}  // namespace Xengine
#endif