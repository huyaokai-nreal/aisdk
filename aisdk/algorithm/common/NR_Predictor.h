/*
 * @Author: Zhang Junsong
 * @Date: 2023-02-01 09:52:22
 * @Last Modified by: Zhang Junsong
 * @Last Modified time: 2023-02-02 01:49:25
 */

#pragma once

#include <memory>
#include <mutex>
#include <opencv2/opencv.hpp>
#include "aisdk/base/type.h"
namespace aisdk::algorithm {

// state variable indices
#define S_X 0
#define S_Y 1
#define S_Z 2
#define S_VX 3
#define S_VY 4
#define S_VZ 5
#define S_AX 6
#define S_AY 7
#define S_AZ 8
// measurement variable indices
#define M_X 0
#define M_Y 1
#define M_Z 2
#define M_VX 3
#define M_VY 4
#define M_VZ 5

// #define BETA Vec3f_t(0.4, 0.4, 0.4)
#define ALPHA \
    Vec3f_t { 0.6, 0.6, 0.6 }
#define ONE \
    Vec3f_t { 1.0, 1.0, 1.0 }

struct PredictorState {
    Vec3f_t pos;
    Vec3f_t vec;
};

class KFPredictor {
   public:
    int init();
    int start_tracking(double target_ts, PredictorState meas);
    void stop_tracking();

    Vec3f_t track_only_pred(double target_ts);
    Vec3f_t track_with_correct(double target_ts, PredictorState meas);

    bool get_tracking_status() const;

   private:
    PredictorState predict(double target_ts);
    PredictorState correct(double target_ts, PredictorState meas, bool restart);
    int m_state_size = 9;
    int m_meas_size = 6;
    int m_ctrl_size = 0;

    double m_time_ts = 0;
    double m_time_ts_last = 0;

    double m_time_ts_p = 0;
    double m_time_ts_last_p = 0;

    unsigned int m_type = CV_32F;

    std::unique_ptr<cv::KalmanFilter> m_kf_impl;

    bool is_tracked = false;

    PredictorState m_momentum;

    mutable std::mutex m_mutex;
};

}