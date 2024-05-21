#pragma once
#include "NR_Predictor.h"
#include "../internal_structs/kpt3d_struct_internal.h"

namespace aisdk::algorithm {

class GlobalPredictorService {
   public:
    static GlobalPredictorService& getInstance() {
        static GlobalPredictorService instance;
        return instance;
    }

    KFPredictor& get_predictor_lhand() { return kfpredictor_lhand; }

    KFPredictor& get_predictor_rhand() { return kfpredictor_rhand; }

    HandsData& get_last_pt3d_world() { return last_kpt3d_world; }
    GlobalPredictorService(const GlobalPredictorService&) = delete;
    GlobalPredictorService& operator=(const GlobalPredictorService&) = delete;

   private:
    KFPredictor kfpredictor_lhand;
    KFPredictor kfpredictor_rhand;

    HandsData last_kpt3d_world;

    GlobalPredictorService() {
        kfpredictor_lhand.init();
        kfpredictor_rhand.init();
    }

    ~GlobalPredictorService() {}

};

}  // namespace aisdk::algorithm
