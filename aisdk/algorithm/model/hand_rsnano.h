#pragma once
#include "calculator_basenet.h"
#include "aisdk/algorithm/func/netalgo_utils.h"

namespace aisdk::algorithm {

#define KEYPOINT_NUM 21
#define OUTPUT_SHAPE 32
#define INPUT_SHAPE 128

struct RSNResult {
    std::vector<std::vector<cv::Vec2f>> rsn_kpts;  // 单手 左目，右目
    std::vector<std::vector<float>> rsn_scores;
};

class RSNTiny : public CalculatorBaseNet {
   public:
    RSNTiny(){};
    ~RSNTiny(){};

    aisdk::xengine::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);
    void PreProcess(const std::vector<Image> &net_input);
    void PostProcess(RSNResult &result);
    void PreProcessSingle(const std::vector<Image> &net_input, uint32_t batchn);
    void PostProcessSingle(RSNResult &result, uint32_t batchn);
    aisdk::xengine::Status Inference(const std::vector<Image> &baseinput, RSNResult &baseresult);

   private:
    aisdk::xengine::TensorFormat itensor_format;
    aisdk::xengine::TensorFormat otensor_format;

    std::vector<float> m_outputsNCHW;

    bool snpe_batch1 = false;
    uint32_t session_batch = 1;
};

}  // namespace aisdk::algorithm