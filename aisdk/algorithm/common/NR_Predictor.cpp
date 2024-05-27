/*
 * @Author: Zhang Junsong
 * @Date: 2023-02-01 09:54:30
 * @Last Modified by: Zhang Junsong
 * @Last Modified time: 2023-02-01 10:10:44
 */
#include "NR_Predictor.h"

#include <memory>
#include <vector>

#include "aisdk/algorithm/common/NR_Seq_Manager.h"
#include "aisdk/base/log.h"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {
int KFPredictor::init() {
    // Init KalmanFilter with status params
    std::lock_guard<std::mutex> lock(m_mutex);
    m_kf_impl = std::make_unique<cv::KalmanFilter>(m_state_size, m_meas_size, m_ctrl_size, m_type);

    // A: Transition State Matrix
    //     x  y  z  vx vy vz ax      ay      az
    // x [ 1  0  0  dT 0  0  0.5dT^2 0       0       ]
    // y [ 0  1  0  0  dT 0  0       0.5dT^2 0       ]
    // z [ 0  0  1  0  0  dT 0       0       0.5dT^2 ]
    // vx[ 0  0  0  1  0  0  dt       0       0       ] -
    // vy[ 0  0  0  0  1  0  0       dt       0       ]  |  ==> Maybe identity * dT
    // yz[ 0  0  0  0  0  1  0       0       dt       ] -
    // ax[ 0  0  0  0  0  0  1       0       0       ]
    // ay[ 0  0  0  0  0  0  0       1       0       ]
    // az[ 0  0  0  0  0  0  0       0       1       ]

    cv::setIdentity(m_kf_impl->transitionMatrix);
    // m_kf_impl->measurementMatrix.at<float>(M_X, S_VX) = 1.0f;
    // m_kf_impl->measurementMatrix.at<float>(M_Y, S_VY) = 1.0f;
    // m_kf_impl->measurementMatrix.at<float>(M_Z, S_VZ) = 1.0f;

    // H: Measurement Matrix
    // 	 x  y  z  vx vy vz ax ay az
    // [ 1  0  0  0  0  0  0  0  0 ] x
    // [ 0  1  0  0  0  0  0  0  0 ] y
    // [ 0  0  1  0  0  0  0  0  0 ] w
    // [ 0  0  0  1  0  0  0  0  0 ] vx
    // [ 0  0  0  0  1  0  0  0  0 ] vy
    // [ 0  0  0  0  0  1  0  0  0 ] vz

    m_kf_impl->measurementMatrix = cv::Mat::zeros(m_meas_size, m_state_size, m_type);
    m_kf_impl->measurementMatrix.at<float>(M_X, S_X) = 1.0f;
    m_kf_impl->measurementMatrix.at<float>(M_Y, S_Y) = 1.0f;
    m_kf_impl->measurementMatrix.at<float>(M_Z, S_Z) = 1.0f;
    m_kf_impl->measurementMatrix.at<float>(M_VX, S_VX) = 1.0f;
    m_kf_impl->measurementMatrix.at<float>(M_VY, S_VY) = 1.0f;
    m_kf_impl->measurementMatrix.at<float>(M_VZ, S_VZ) = 1.0f;

    //   Q: Process Noise Covariance Matrix
    //   x    y    z    vx   vy   vz   ax   ay   az
    // [ E_x  0    0    0    0    0    0    0    0    ]
    // [ 0    E_y  0    0    0    0    0    0    0    ]
    // [ 0    0    E_z  0    0    0    0    0    0    ]
    // [ 0    0    0    E_vx 0    0    0    0    0    ]
    // [ 0    0    0    0    E_vy 0    0    0    0    ]
    // [ 0    0    0    0    0    E_vz 0    0    0    ]
    // [ 0    0    0    0    0    0    E_ax 0    0    ]
    // [ 0    0    0    0    0    0    0    E_ay 0    ]
    // [ 0    0    0    0    0    0    0    0    E_az ]

    cv::setIdentity(m_kf_impl->processNoiseCov, cv::Scalar(1e-1));
    // Override velocity errors
    m_kf_impl->processNoiseCov.at<float>(S_X, S_X) = 1.0;
    m_kf_impl->processNoiseCov.at<float>(S_Y, S_Y) = 1.0;
    m_kf_impl->processNoiseCov.at<float>(S_Z, S_Z) = 1.0;

    cv::setIdentity(m_kf_impl->measurementNoiseCov, cv::Scalar(1e-4));

    // m_kf_impl->measurementNoiseCov.at<float>(S_X, S_X) = 5e-2;
    // m_kf_impl->measurementNoiseCov.at<float>(S_Y, S_Y) = 5e-2;
    // m_kf_impl->measurementNoiseCov.at<float>(S_Z, S_Z) = 1e-3;

    // cv::setIdentity(m_kf_impl->errorCovPost, cv::Scalar(.1));
    // init smoother
    reset_predict_smoother();

    return 0;
}
void KFPredictor::reset_predict_smoother() {
    OneEuroParams center_params;
    if (glasses_type_ == "flora") {
        center_params.mincutoff = {0.1, 0.1, 0.1};  // 调静止状态下的稳定性,越小稳定性越好
        center_params.beta = {20.0, 20.0, 20.0};  // 运动状态下alpha的变化速率，alpha越大，跟踪越及时
        center_params.dcutoff = {0.8, 0.8, 0.5};  // 速度滤波的固定效果
        predict_length_ratio_ = 1.0;
    } else if (glasses_type_ == "ella") {
        center_params.mincutoff = {0.1, 0.1, 0.1};  // 调静止状态下的稳定性,越小稳定性越好
        center_params.beta = {10.0, 10.0, 10.0};  // 运动状态下alpha的变化速率，alpha越大，跟踪越及时
        center_params.dcutoff = {0.8, 0.8, 0.5};  // 速度滤波的固定效果
        predict_length_ratio_ = 1.0;
    } else {
        AISDK_LOG_ERROR("[KFPredict]: undedfined glasses type {}", glasses_type_);
    }

    center_params.freq = 60;
    predict_smoother_ = std::make_unique<SeqManager3D>(1, center_params);
}
int KFPredictor::start_tracking(double target_ts, PredictorState meas) {
    std::lock_guard<std::mutex> lock(m_mutex);
    cv::Mat state = cv::Mat::zeros(m_state_size, 1, m_type);
    state.at<float>(S_X) = meas.pos[0];
    state.at<float>(S_Y) = meas.pos[1];
    state.at<float>(S_Z) = meas.pos[2];
    state.at<float>(S_VX) = meas.vec[0];
    state.at<float>(S_VY) = meas.vec[1];
    state.at<float>(S_VZ) = meas.vec[2];

    m_kf_impl->statePost = state;
    m_kf_impl->statePre = state;
    last_correct_time_ = target_ts;
    m_momentum.pos = meas.pos;
    reset_predict_smoother();
    is_tracked = true;
    return 0;
}
void KFPredictor::update_transition_matrix(double target_ts) {
    double dt_seconds = target_ts - last_correct_time_;
    m_kf_impl->transitionMatrix.at<float>(S_X, S_VX) = dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Y, S_VY) = dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Z, S_VZ) = dt_seconds;

    m_kf_impl->transitionMatrix.at<float>(S_VX, S_AX) = abs(dt_seconds);
    m_kf_impl->transitionMatrix.at<float>(S_VY, S_AY) = abs(dt_seconds);
    m_kf_impl->transitionMatrix.at<float>(S_VZ, S_AZ) = abs(dt_seconds);

    m_kf_impl->transitionMatrix.at<float>(S_X, S_AX) = 0.5 * abs(dt_seconds) * dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Y, S_AY) = 0.5 * abs(dt_seconds) * dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Z, S_AZ) = 0.5 * abs(dt_seconds) * dt_seconds;
}

PredictorState KFPredictor::predict() {
    m_kf_impl->predict();
    return {Vec3f_t{m_kf_impl->statePre.at<float>(S_X), m_kf_impl->statePre.at<float>(S_Y),
                    m_kf_impl->statePre.at<float>(S_Z)},
            Vec3f_t{m_kf_impl->statePre.at<float>(S_VX), m_kf_impl->statePre.at<float>(S_VY),
                    m_kf_impl->statePre.at<float>(S_VZ)}};
}

PredictorState KFPredictor::correct(PredictorState meas) {
    cv::Mat meas_mat = cv::Mat::zeros(m_meas_size, 1, m_type);
    meas_mat.at<float>(M_X) = meas.pos[0];
    meas_mat.at<float>(M_Y) = meas.pos[1];
    meas_mat.at<float>(M_Z) = meas.pos[2];

    meas_mat.at<float>(M_VX) = meas.vec[0];
    meas_mat.at<float>(M_VY) = meas.vec[1];
    meas_mat.at<float>(M_VZ) = meas.vec[2];
    m_kf_impl->correct(meas_mat);

    auto pred_pos = Vec3f_t{m_kf_impl->statePost.at<float>(S_X), m_kf_impl->statePost.at<float>(S_Y),
                            m_kf_impl->statePost.at<float>(S_Z)};
    auto pred_vec = Vec3f_t{m_kf_impl->statePost.at<float>(S_VX), m_kf_impl->statePost.at<float>(S_VY),
                            m_kf_impl->statePost.at<float>(S_VZ)};

    m_momentum.pos = ALPHA.array() * m_momentum.pos.array() + (ONE - ALPHA).array() * pred_pos.array();

    return {m_momentum.pos, pred_vec};
}

Vec3f_t KFPredictor::track_only_pred(double target_ts, bool with_smooth) {
    double valid_target_ts = get_valid_predict_time_length(target_ts);
    std::lock_guard<std::mutex> lock(m_mutex);
    update_transition_matrix(valid_target_ts);
    auto pred = this->predict();
    std::vector<Vec3f_t> pred_pose{pred.pos};
    // this->correct(pred);
    // last_correct_time_ = target_ts;
    if (with_smooth) {
        predict_smoother_->getFilterHandData(pred_pose);
    }
    return pred_pose[0];
}

double KFPredictor::get_valid_predict_time_length(double target_ts) {
    std::array<double, 3> predict_time_interval_vec = {0.1, 0.08, 0.04};  // 100ms, 80ms, 40ms
    constexpr float static_hand_th_1 = 0.1;
    constexpr float static_hand_th_2 = 0.2;
    int hand_static_state = 0;  // 0 dynamic, 1 middle, 2 static
    auto hand_speed = Vec3f_t{m_kf_impl->statePost.at<float>(S_VX), m_kf_impl->statePost.at<float>(S_VY),
                              m_kf_impl->statePost.at<float>(S_VZ)}
                          .norm();
    if (hand_speed < static_hand_th_1) {
        hand_static_state = 2;
    } else if (hand_speed < static_hand_th_2) {
        hand_static_state = 1;
    }
    double target_timestamp = 0;
    auto predict_interval = (target_ts - last_measure_time_) * predict_length_ratio_;
    predict_interval = std::min(predict_interval, predict_time_interval_vec[hand_static_state]);
    // if ((m_kf_impl->statePost.at<float>(S_AX) < -0.1) || (m_kf_impl->statePost.at<float>(S_AY) < -0.1)) {
    //     predict_interval = std::min(predict_interval, 0.02);
    // }
    // if ((m_kf_impl->statePost.at<float>(S_AX) < -0.2) || (m_kf_impl->statePost.at<float>(S_AY) < -0.2)) {
    //     predict_interval = std::min(predict_interval, 0.01);
    // }
    target_timestamp = predict_interval + last_measure_time_;
    return target_timestamp;
}
Vec3f_t KFPredictor::track_with_correct(double target_ts, PredictorState meas) {
    std::lock_guard<std::mutex> lock(m_mutex);
    update_transition_matrix(target_ts);
    auto pred = this->predict();
    auto cpred = this->correct(meas);
    last_correct_time_ = target_ts;
    last_measure_time_ = target_ts;

    return cpred.pos;
}

bool KFPredictor::get_tracking_status() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return is_tracked;
}

void KFPredictor::stop_tracking() {
    std::lock_guard<std::mutex> lock(m_mutex);
    is_tracked = false;
}

}  // namespace aisdk::algorithm
