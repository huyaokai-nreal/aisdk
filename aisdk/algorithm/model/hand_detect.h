#pragma once

#include "../internal_structs/det_struct_internal.h"
#include "aisdk/xengine/nr_model_mgr.h"
#include "calculator_basenet.h"

#define FEATURE_NUM 7

namespace aisdk::algorithm {

class HandDetectNet : public CalculatorBaseNet {
   public:
    struct GridAnchor {
        float grid_x;
        float grid_y;
        float anchor_rw;
        float anchor_rh;
    };

    HandDetectNet() : CalculatorBaseNet(){};
    ~HandDetectNet(){};

    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);
    void PreProcess(const std::vector<Image> &net_input);
    void PostProcess(DetOutputInternal &result);
    void PreProcessSingle(const std::vector<Image> &net_input, uint32_t batchn);
    void PostProcessSingle(DetOutputInternal &result, uint32_t batchn);
    absl::Status Inference(const std::vector<Image> &baseinput, DetOutputInternal &baseresult);

   protected:
    aisdk::xengine::TensorFormat itensor_format;
    aisdk::xengine::TensorFormat otensor_format;

    std::vector<GridAnchor> grid_anchor;
    int origin_img_width;
    int origin_img_height;

    float score_threshold = 0.5f;
    float iou_threshold = 0.45f;

    uint32_t grid_stride = 16;
    uint32_t grid_w = 16;
    uint32_t grid_h = 12;

    bool net_batch1 = false;
    uint32_t session_batch = 1;

    bool export_netalgo_exec_info = false;
};

/*
    Det v2.0 从单输出变成双输出
    - output_cls [B, 3, 16, 12] (3维依次是conf, left_cls, right_cls)
    - output_box [B, 4, 16, 12] (4维依次是x1,y1,x2,y2)
*/
class HandDetectNetv2 : public HandDetectNet {
   public:
    HandDetectNetv2() : HandDetectNet(){};
    ~HandDetectNetv2(){};

    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);
    absl::Status Inference(const std::vector<Image> &baseinput, DetOutputInternal &baseresult);
    void PostProcess(DetOutputInternal &result);
    // void PostProcessSingle(DetOutputInternal &result, uint32_t batchn);  // TODO: develop中的 PostProcessSingle
    // 目前就还没改
    void myprint();
};

}  // namespace aisdk::algorithm
