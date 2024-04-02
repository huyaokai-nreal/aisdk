/*
 * @Author: Zhang Junsong
 * @Date: 2023-02-01 09:54:30
 * @Last Modified by: Zhang Junsong
 * @Last Modified time: 2023-02-01 10:10:44
 */
#include "NR_Predictor.h"

#include <algorithm>

#include "aisdk/base/log.h"

namespace aisdk::algorithm {
int KFPredictor::init() {
    // Init KalmanFilter with status params
    std::lock_guard<std::mutex> lock(m_mutex);
    m_kf_impl = std::make_unique<cv::KalmanFilter>(m_state_size, m_meas_size, m_ctrl_size, m_type);

    // A: Transition State Matrix
    //   x  y  z  vx vy vz ax      ay      az
    // [ 1  0  0  dT 0  0  0.5dT^2 0       0       ]
    // [ 0  1  0  0  dT 0  0       0.5dT^2 0       ]
    // [ 0  0  1  0  0  dT 0       0       0.5dT^2 ]
    // [ 0  0  0  1  0  0  0       0       0       ] -
    // [ 0  0  0  0  1  0  0       0       0       ]  |  ==> Maybe identity * dT
    // [ 0  0  0  0  0  1  0       0       0       ] -
    // [ 0  0  0  0  0  0  1       0       0       ]
    // [ 0  0  0  0  0  0  0       1       0       ]
    // [ 0  0  0  0  0  0  0       0       1       ]

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

    return 0;
}

int KFPredictor::start_tracking(uint64_t target_ts, PredictorState meas) {
    // //AISDK_LOG_INFO("start_tracking start");
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
    m_time_ts_last = m_time_ts;
    m_time_ts = target_ts;

    m_momentum.pos = meas.pos;

    is_tracked = true;
    return 0;
}

PredictorState KFPredictor::predict(uint64_t target_ts) {
    // update time status
    // m_time_ts_last = m_time_ts;
    // m_time_ts = target_ts;
    int sign = (target_ts < m_time_ts) ? -1 : 1;

    // AISDK_LOG_INFO("sign: {}", sign);

    double dt_seconds = (double(sign * (target_ts - m_time_ts))) / 1000000000.;

    // AISDK_LOG_INFO("target_ts_1: {}, m_time_ts: {}, dt: {}", target_ts, m_time_ts, dt_seconds);
    // AISDK_LOG_INFO("long: {}", target_ts - m_time_ts);
    // AISDK_LOG_INFO("double cast: {}", static_cast<double>(sign * (target_ts - m_time_ts)));
    // AISDK_LOG_INFO("double: {}", double(sign * (target_ts - m_time_ts)));
    // AISDK_LOG_INFO("target_ts_2: {}, m_time_ts: {}, dt: {}", target_ts, m_time_ts, dt_seconds);

    m_kf_impl->transitionMatrix.at<float>(S_X, S_VX) = sign * dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Y, S_VY) = sign * dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Z, S_VZ) = sign * dt_seconds;

    m_kf_impl->transitionMatrix.at<float>(S_VX, S_AX) = dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_VY, S_AY) = dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_VZ, S_AZ) = dt_seconds;

    m_kf_impl->transitionMatrix.at<float>(S_X, S_AX) = sign * 0.5 * dt_seconds * dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Y, S_AY) = sign * 0.5 * dt_seconds * dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Z, S_AZ) = sign * 0.5 * dt_seconds * dt_seconds;

    m_kf_impl->predict();

    // AISDK_LOG_INFO("predict state: {}, {}, {}, {}, {}, {}", m_kf_impl->statePre.at<float>(S_X),
    //                m_kf_impl->statePre.at<float>(S_Y), m_kf_impl->statePre.at<float>(S_Z),
    //                m_kf_impl->statePre.at<float>(S_VX), m_kf_impl->statePre.at<float>(S_VY),
    //                m_kf_impl->statePre.at<float>(S_VZ));

    return {cv::Vec3f{m_kf_impl->statePre.at<float>(S_X), m_kf_impl->statePre.at<float>(S_Y),
                      m_kf_impl->statePre.at<float>(S_Z)},
            cv::Vec3f{m_kf_impl->statePre.at<float>(S_VX), m_kf_impl->statePre.at<float>(S_VY),
                      m_kf_impl->statePre.at<float>(S_VZ)}};
}

PredictorState KFPredictor::correct(uint64_t target_ts, PredictorState meas, bool restart) {
    if (restart) {
        m_time_ts = target_ts;
        cv::setIdentity(m_kf_impl->errorCovPre);

        // Updating statePost with bbx, bby, bbw, and bbh
        m_kf_impl->statePost.at<float>(S_X) = meas.pos[0];
        m_kf_impl->statePost.at<float>(S_Y) = meas.pos[1];
        m_kf_impl->statePost.at<float>(S_Z) = meas.pos[2];

        m_kf_impl->statePost.at<float>(S_VX) = meas.vec[0];
        m_kf_impl->statePost.at<float>(S_VY) = meas.vec[1];
        m_kf_impl->statePost.at<float>(S_VZ) = meas.vec[2];

    } else {
        m_time_ts_last = m_time_ts;
        m_time_ts = target_ts;

        int sign = (m_time_ts < m_time_ts_last) ? -1 : 1;

        float dt_seconds = (double(sign * (m_time_ts - m_time_ts_last))) / 1000000000.;

        cv::Mat meas_mat = cv::Mat::zeros(m_meas_size, 1, m_type);
        meas_mat.at<float>(M_X) = meas.pos[0];
        meas_mat.at<float>(M_Y) = meas.pos[1];
        meas_mat.at<float>(M_Z) = meas.pos[2];

        meas_mat.at<float>(M_VX) = meas.vec[0];
        meas_mat.at<float>(M_VY) = meas.vec[1];
        meas_mat.at<float>(M_VZ) = meas.vec[2];

        // AISDK_LOG_INFO("meas target_ts: {}, m_time_ts: {}, dt: {}", m_time_ts, m_time_ts_last, dt_seconds);

        // AISDK_LOG_INFO("meas state 1: {}, {}, {}, {}, {}, {}", meas.pos[0], meas.pos[1], meas.pos[2],
        //                       meas.vec[0], meas.vec[1], meas.vec[2]);

        m_kf_impl->transitionMatrix.at<float>(S_X, S_VX) = sign * dt_seconds;
        m_kf_impl->transitionMatrix.at<float>(S_Y, S_VY) = sign * dt_seconds;
        m_kf_impl->transitionMatrix.at<float>(S_Z, S_VZ) = sign * dt_seconds;

        m_kf_impl->transitionMatrix.at<float>(S_VX, S_AX) = dt_seconds;
        m_kf_impl->transitionMatrix.at<float>(S_VY, S_AY) = dt_seconds;
        m_kf_impl->transitionMatrix.at<float>(S_VZ, S_AZ) = dt_seconds;

        m_kf_impl->transitionMatrix.at<float>(S_X, S_AX) = sign * 0.5 * dt_seconds * dt_seconds;
        m_kf_impl->transitionMatrix.at<float>(S_Y, S_AY) = sign * 0.5 * dt_seconds * dt_seconds;
        m_kf_impl->transitionMatrix.at<float>(S_Z, S_AZ) = sign * 0.5 * dt_seconds * dt_seconds;

        m_kf_impl->correct(meas_mat);

        // AISDK_LOG_INFO("meas state 2: {}, {}, {}, {}, {}, {}", m_kf_impl->statePost.at<float>(M_X),
        //                       m_kf_impl->statePost.at<float>(M_Y), m_kf_impl->statePost.at<float>(M_Z),
        //                       m_kf_impl->statePost.at<float>(M_VX), m_kf_impl->statePost.at<float>(M_VY),
        //                       m_kf_impl->statePost.at<float>(M_VZ));
    }

    // AISDK_LOG_INFO("meas predicted a: {}, {}, {}", m_kf_impl->statePost.at<float>(S_AX),
    //                       m_kf_impl->statePost.at<float>(S_AY), m_kf_impl->statePost.at<float>(S_AZ));

    auto pred_pos = cv::Vec3f{m_kf_impl->statePost.at<float>(S_X), m_kf_impl->statePost.at<float>(S_Y),
                              m_kf_impl->statePost.at<float>(S_Z)};
    auto pred_vec = cv::Vec3f{m_kf_impl->statePost.at<float>(S_VX), m_kf_impl->statePost.at<float>(S_VY),
                              m_kf_impl->statePost.at<float>(S_VZ)};

    m_momentum.pos = ALPHA * m_momentum.pos + (ONE - ALPHA) * pred_pos;

    return {m_momentum.pos, pred_vec};
}

cv::Vec3f KFPredictor::track_only_pred(uint64_t target_ts) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto pred = this->predict(target_ts);
    auto cpred = this->correct(target_ts, pred, false);

    return pred.pos;
}

cv::Vec3f KFPredictor::track_with_correct(uint64_t target_ts, PredictorState meas) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto pred = this->predict(target_ts);
    auto cpred = this->correct(target_ts, meas, false);

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
