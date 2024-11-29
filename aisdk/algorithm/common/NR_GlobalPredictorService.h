#pragma once
#include <mutex>
#include "../internal_structs/kpt2d_struct_internal.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "NR_Predictor.h"
#include "NR_Predictor2d.h"

namespace aisdk::algorithm {

class GlobalPredictorService {
   public:
    static GlobalPredictorService& getInstance() {
        static GlobalPredictorService instance;
        return instance;
    }

    KFPredictor& get_predictor_lhand() { return kfpredictor_lhand; }

    KFPredictor& get_predictor_rhand() { return kfpredictor_rhand; }
    
    KFPredictor2d& get_predictor_lhand_lcam_bbox() { return bboxpredictor_lhand_lcam; }
    KFPredictor2d& get_predictor_rhand_lcam_bbox() { return bboxpredictor_rhand_lcam; }
    KFPredictor2d& get_predictor_lhand_rcam_bbox() { return bboxpredictor_lhand_rcam; }
    KFPredictor2d& get_predictor_rhand_rcam_bbox() { return bboxpredictor_rhand_rcam; }

    Kpt2dInternal get_last_kpt2d_pixel() { return last_kpt2d_pixel; }
    HandsData get_last_kpt3d_world() { return last_kpt3d_world; }

    void set_last_kpt2d_pixel(const Kpt2dInternal& data) { last_kpt2d_pixel = data; }
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
    KFPredictor2d bboxpredictor_lhand_lcam;
    KFPredictor2d bboxpredictor_rhand_lcam;
    KFPredictor2d bboxpredictor_lhand_rcam;
    KFPredictor2d bboxpredictor_rhand_rcam;

    float hand_scale_{1.0};
    float hand_scale_alpha_ = 0.01;
    Kpt2dInternal last_kpt2d_pixel;
    HandsData last_kpt3d_world;

    GlobalPredictorService() {
        kfpredictor_lhand.init();
        kfpredictor_rhand.init();
        bboxpredictor_lhand_lcam.init();
        bboxpredictor_rhand_lcam.init();
        bboxpredictor_lhand_rcam.init();
        bboxpredictor_rhand_rcam.init();
    }

    ~GlobalPredictorService() {}
};

}  // namespace aisdk::algorithm
