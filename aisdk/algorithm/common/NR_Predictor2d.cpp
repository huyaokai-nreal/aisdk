/*
 * @Author: Zhang Junsong
 * @Date: 2023-02-01 09:54:30
 * @Last Modified by: Zhang Junsong
 * @Last Modified time: 2023-02-01 10:10:44
 */
#include "NR_Predictor2d.h"

#include <memory>
#include <utility>
#include <vector>

#include "aisdk/algorithm/common/NR_Seq_Manager.h"
#include "aisdk/base/log.h"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {
int KFPredictor2d::init() {
    // Init KalmanFilter with status params
    std::lock_guard<std::mutex> lock(m_mutex);
    m_kf_impl = std::make_unique<cv::KalmanFilter>(m_state_size, m_meas_size, m_ctrl_size, m_type);
    // reset kalman filter
    reset_kalman_fileter();

    return 0;
}
void KFPredictor2d::reset_kalman_fileter() {
    // A: Transition State Matrix
    //     x  y  vx vy ax      ay
    // x [ 1  0  dT 0  0.5dT^2 0       ]
    // y [ 0  1  0  dT  0       0.5dT^2 ]
    // vx[ 0  0  1  0  dt       0      ] -
    // vy[ 0  0  0  1  0       dt       ]  |  ==> Maybe identity * dT
    // ax[ 0  0  0  0  1       0       ]
    // ay[ 0  0  0  0  0       1       ]

    cv::setIdentity(m_kf_impl->transitionMatrix);
    // m_kf_impl->measurementMatrix.at<float>(M_X, S_VX) = 1.0f;
    // m_kf_impl->measurementMatrix.at<float>(M_Y, S_VY) = 1.0f;

    // H: Measurement Matrix
    // 	 x  y  vx vy ax ay
    // [ 1  0  0  0  0  0 ] x
    // [ 0  1  0  0  0  0 ] y
    // [ 0  0  1  0  0  0 ] vx
    // [ 0  0  0  1  0  0 ] vy

    m_kf_impl->measurementMatrix = cv::Mat::zeros(m_meas_size, m_state_size, m_type);
    m_kf_impl->measurementMatrix.at<float>(M_X, S_X) = 1.0f;
    m_kf_impl->measurementMatrix.at<float>(M_Y, S_Y) = 1.0f;
    m_kf_impl->measurementMatrix.at<float>(M_VX, S_VX) = 1.0f;
    m_kf_impl->measurementMatrix.at<float>(M_VY, S_VY) = 1.0f;

    //   Q: Process Noise Covariance Matrix
    //   x    y    vx   vy   ax   ay
    // [ E_x  0    0    0    0    0    ]
    // [ 0    E_y  0    0    0    0    ]
    // [ 0    0    0    0    0    0    ]
    // [ 0    0    E_vx 0    0    0    ]
    // [ 0    0    0    E_vy 0    0    ]
    // [ 0    0    0    0    0    0    ]
    // [ 0    0    0    0    E_ax 0    ]
    // [ 0    0    0    0    0    E_ay ]
    // [ 0    0    0    0    0    0    ]

    cv::setIdentity(m_kf_impl->processNoiseCov, cv::Scalar(1e-1));
    // Override velocity errors
    m_kf_impl->processNoiseCov.at<float>(S_X, S_X) = 1.0;
    m_kf_impl->processNoiseCov.at<float>(S_Y, S_Y) = 1.0;

    cv::setIdentity(m_kf_impl->measurementNoiseCov, cv::Scalar(1e-4));
}

int KFPredictor2d::start_tracking(double target_ts, PredictorState_2d meas) {
    std::lock_guard<std::mutex> lock(m_mutex);
    reset_kalman_fileter();
    cv::Mat state = cv::Mat::zeros(m_state_size, 1, m_type);
    state.at<float>(S_X) = meas.pos[0];
    state.at<float>(S_Y) = meas.pos[1];
    state.at<float>(S_VX) = meas.vec[0];
    state.at<float>(S_VY) = meas.vec[1];

    m_kf_impl->statePost = state;
    m_kf_impl->statePre = state;
    last_correct_time_ = target_ts;
    last_measure_time_ = target_ts;
    m_momentum.pos = meas.pos;
    is_tracked = true;
    return 0;
}
void KFPredictor2d::update_transition_matrix(double target_ts) {
    double dt_seconds = target_ts - last_correct_time_;
    m_kf_impl->transitionMatrix.at<float>(S_X, S_VX) = dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Y, S_VY) = dt_seconds;

    m_kf_impl->transitionMatrix.at<float>(S_VX, S_AX) = abs(dt_seconds);
    m_kf_impl->transitionMatrix.at<float>(S_VY, S_AY) = abs(dt_seconds);

    m_kf_impl->transitionMatrix.at<float>(S_X, S_AX) = 0.5 * abs(dt_seconds) * dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Y, S_AY) = 0.5 * abs(dt_seconds) * dt_seconds;
}

PredictorState_2d KFPredictor2d::predict() {
    m_kf_impl->predict();
    return {Vec2f_t{m_kf_impl->statePre.at<float>(S_X), m_kf_impl->statePre.at<float>(S_Y)},
            Vec2f_t{m_kf_impl->statePre.at<float>(S_VX), m_kf_impl->statePre.at<float>(S_VY)}};
}

PredictorState_2d KFPredictor2d::correct(PredictorState_2d meas) {
    cv::Mat meas_mat = cv::Mat::zeros(m_meas_size, 1, m_type);
    meas_mat.at<float>(M_X) = meas.pos[0];
    meas_mat.at<float>(M_Y) = meas.pos[1];

    meas_mat.at<float>(M_VX) = meas.vec[0];
    meas_mat.at<float>(M_VY) = meas.vec[1];
    m_kf_impl->correct(meas_mat);

    auto pred_pos = Vec2f_t{m_kf_impl->statePost.at<float>(S_X), m_kf_impl->statePost.at<float>(S_Y)};
    auto pred_vec = Vec2f_t{m_kf_impl->statePost.at<float>(S_VX), m_kf_impl->statePost.at<float>(S_VY)};

    m_momentum.pos = ALPHA_2D.array() * m_momentum.pos.array() + (ONE_2D - ALPHA_2D).array() * pred_pos.array();

    return {m_momentum.pos, pred_vec};
}
Vec2f_t KFPredictor2d::track_with_correct(double target_ts, PredictorState_2d meas) {
    std::lock_guard<std::mutex> lock(m_mutex);
    update_transition_matrix(target_ts);
    auto pred = this->predict();
    auto cpred = this->correct(std::move(meas));
    last_correct_time_ = target_ts;
    last_measure_time_ = target_ts;

    return cpred.pos;
}

Vec2f_t KFPredictor2d::track_only_pred(double target_ts, bool update_state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    double valid_target_ts = get_valid_predict_time_length(target_ts);
    update_transition_matrix(valid_target_ts);
    std::vector<Vec2f_t> pred_pose;
    PredictorState_2d pred;
    if (update_state) {
        pred = this->predict();
        pred_pose.emplace_back(pred.pos);
        this->correct(pred);
        last_correct_time_ = target_ts;
    } else {
        cv::Mat pred_state = m_kf_impl->transitionMatrix * m_kf_impl->statePost;
        pred_pose.emplace_back(pred_state.at<float>(S_X), pred_state.at<float>(S_Y));
    }

    return pred_pose[0];
}

double KFPredictor2d::get_valid_predict_time_length(double target_ts) {
    std::array<double, 3> predict_time_interval_vec = {0.1, 0.08, 0.04};  // 100ms, 80ms, 40ms
    constexpr float static_hand_th_1 = 0.1;
    constexpr float static_hand_th_2 = 0.2;
    int hand_static_state = 0;  // 0 dynamic, 1 middle, 2 static
    auto hand_speed = Vec2f_t{m_kf_impl->statePost.at<float>(S_VX), m_kf_impl->statePost.at<float>(S_VY)}.norm();
    if (hand_speed < static_hand_th_1) {
        hand_static_state = 2;
    } else if (hand_speed < static_hand_th_2) {
        hand_static_state = 1;
    }
    double target_timestamp = 0;
    auto predict_interval = (target_ts - last_measure_time_) * predict_length_ratio_;
    predict_interval = std::min(predict_interval, predict_time_interval_vec[hand_static_state]);
    target_timestamp = predict_interval + last_measure_time_;
    return target_timestamp;
}

bool KFPredictor2d::get_tracking_status() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return is_tracked;
}

void KFPredictor2d::stop_tracking() {
    std::lock_guard<std::mutex> lock(m_mutex);
    is_tracked = false;
}

}  // namespace aisdk::algorithm
