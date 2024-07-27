#pragma once
#include <mutex>
#include "../internal_structs/kpt3d_struct_internal.h"
#include "NR_Predictor.h"

namespace aisdk::algorithm {

class GlobalPredictorService {
   public:
    static GlobalPredictorService& getInstance() {
        static GlobalPredictorService instance;
        return instance;
    }

    KFPredictor& get_predictor_lhand() { return kfpredictor_lhand; }

    KFPredictor& get_predictor_rhand() { return kfpredictor_rhand; }
    KFPredictor& get_predictor_lhand_bbox() { return bboxpredictor_lhand; }

    KFPredictor& get_predictor_rhand_bbox() { return bboxpredictor_rhand; }

    HandsData get_last_kpt3d_world() { return last_kpt3d_world; }
    void set_last_kpt3d_world(const HandsData& data) { last_kpt3d_world = data; }
    GlobalPredictorService(const GlobalPredictorService&) = delete;
    GlobalPredictorService& operator=(const GlobalPredictorService&) = delete;
    void update_hand_scale(float hand_scale) {
        hand_scale_ = (1 - hand_scale_alpha_) * hand_scale_ + hand_scale_alpha_ * hand_scale;
    }
    float get_hand_scale() const { return hand_scale_; }

   private:
    KFPredictor kfpredictor_lhand;
    KFPredictor kfpredictor_rhand;
    KFPredictor bboxpredictor_lhand;
    KFPredictor bboxpredictor_rhand;
    float hand_scale_{1.0};
    float hand_scale_alpha_ = 0.01;
    HandsData last_kpt3d_world;

    GlobalPredictorService() {
        kfpredictor_lhand.init();
        kfpredictor_rhand.init();
        bboxpredictor_lhand.init();
        bboxpredictor_rhand.init();
    }

    ~GlobalPredictorService() {}
};

}  // namespace aisdk::algorithm
