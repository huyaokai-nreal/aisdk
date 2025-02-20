/*
 * @Author: Zhang Junsong
 * @Date: 2023-02-01 09:54:30
 * @Last Modified by: Zhang Junsong
 * @Last Modified time: 2023-02-01 10:10:44
 */
#include "NR_Predictor.h"

#include <memory>
#include <utility>
#include <vector>

#include "aisdk/algorithm/common/NR_Seq_Manager.h"
#include "aisdk/base/log.h"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {

/// @brief 初始化函数
/// @return 0
int KFPredictor::init() {
    // Init KalmanFilter with status params
    std::lock_guard<std::mutex> lock(m_mutex);
    m_kf_impl = std::make_unique<cv::KalmanFilter>(m_state_size, m_meas_size, m_ctrl_size, m_type);

    //重置卡尔曼滤波器
    reset_kalman_fileter();

    //重置3d平滑器
    reset_predict_smoother();

    return 0;
}

/// @brief 重置卡尔曼滤波器
void KFPredictor::reset_kalman_fileter() {
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

    //重置状态转移矩阵（用于描述运动过程，目前初始化为单位矩阵）
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

    //设置测量矩阵（仅测量位置 (x, y, z) 和速度 (vx, vy, vz)，不测量加速度）
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

    //过程噪声协方差矩阵（表示系统的不确定性，即状态变量随时间推移的误差。明确位置变量的噪声协方差设为 1.0，说明位置状态的不确定性较大）
    cv::setIdentity(m_kf_impl->processNoiseCov, cv::Scalar(1e-1));
    // Override velocity errors
    m_kf_impl->processNoiseCov.at<float>(S_X, S_X) = 1.0;
    m_kf_impl->processNoiseCov.at<float>(S_Y, S_Y) = 1.0;
    m_kf_impl->processNoiseCov.at<float>(S_Z, S_Z) = 1.0;

    //测量噪声协方差矩阵（表示测量误差的协方差，通常与传感器精度相关。这里设置成
    // 1e-4，意味着测量噪声较小，传感器比较精准）
    cv::setIdentity(m_kf_impl->measurementNoiseCov, cv::Scalar(1e-4));
}

/// @brief 重置3d预测平滑器
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

    //设置平滑器频率参数
    center_params.freq = 60;
    predict_smoother_ = std::make_unique<SeqManager3D>(1, center_params);
}

/// @brief 开始进行轨迹追踪和预测
/// @param target_ts 预测的时间点
/// @param meas 传进来的初始测量值
/// @return 0
int KFPredictor::start_tracking(double target_ts, PredictorState meas) {
    std::lock_guard<std::mutex> lock(m_mutex);

    //重置卡尔曼滤波器
    reset_kalman_fileter();

    //将测量到的目标信息赋值给卡尔曼滤波器
    cv::Mat state = cv::Mat::zeros(m_state_size, 1, m_type);
    state.at<float>(S_X) = meas.pos[0];
    state.at<float>(S_Y) = meas.pos[1];
    state.at<float>(S_Z) = meas.pos[2];
    state.at<float>(S_VX) = meas.vec[0];
    state.at<float>(S_VY) = meas.vec[1];
    state.at<float>(S_VZ) = meas.vec[2];

    //以初始测量值作为初始状态，进行赋值
    m_kf_impl->statePost = state;
    m_kf_impl->statePre = state;
    last_correct_time_ = target_ts;
    last_measure_time_ = target_ts;
    m_momentum.pos = meas.pos;
    predict_smoother_->reset();

    //正在被跟踪
    is_tracked = true;
    return 0;
}

/// @brief 更新卡尔曼滤波器的状态转移矩阵
/// @param target_ts 预测时间
void KFPredictor::update_transition_matrix(double target_ts) {
    //当前时间戳与start_tracking时间戳差值，计算时间间隔
    double dt_seconds = target_ts - last_correct_time_;

    //设定位置和速度之间关系（S_X_NEW = S_X_OLD + S_VX * dt_seconds）
    m_kf_impl->transitionMatrix.at<float>(S_X, S_VX) = dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Y, S_VY) = dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Z, S_VZ) = dt_seconds;

    //设定速度和加速度之间的关系（S_VX_NEW = S_VX_OLD + S_AX * dt_seconds）
    m_kf_impl->transitionMatrix.at<float>(S_VX, S_AX) = abs(dt_seconds);
    m_kf_impl->transitionMatrix.at<float>(S_VY, S_AY) = abs(dt_seconds);
    m_kf_impl->transitionMatrix.at<float>(S_VZ, S_AZ) = abs(dt_seconds);

    //设定位置和加速度之间的关系（S_X_NEW = S_X_OLD + S_AX * 0.5 * dt_seconds ^ 2）
    m_kf_impl->transitionMatrix.at<float>(S_X, S_AX) = 0.5 * abs(dt_seconds) * dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Y, S_AY) = 0.5 * abs(dt_seconds) * dt_seconds;
    m_kf_impl->transitionMatrix.at<float>(S_Z, S_AZ) = 0.5 * abs(dt_seconds) * dt_seconds;
}

/// @brief 进行位置预测，并返回结果
/// @return 返回预测的位置信息
PredictorState KFPredictor::predict() {
    m_kf_impl->predict();
    return {Vec3f_t{m_kf_impl->statePre.at<float>(S_X), m_kf_impl->statePre.at<float>(S_Y),
                    m_kf_impl->statePre.at<float>(S_Z)},
            Vec3f_t{m_kf_impl->statePre.at<float>(S_VX), m_kf_impl->statePre.at<float>(S_VY),
                    m_kf_impl->statePre.at<float>(S_VZ)}};
}

/// @brief 使用测量值校正卡尔曼滤波器的状态
/// @param meas 测量值信息
/// @return 返回修正后的预测状态
PredictorState KFPredictor::correct(PredictorState meas) {
    //创建测量矩阵并赋值
    cv::Mat meas_mat = cv::Mat::zeros(m_meas_size, 1, m_type);
    meas_mat.at<float>(M_X) = meas.pos[0];
    meas_mat.at<float>(M_Y) = meas.pos[1];
    meas_mat.at<float>(M_Z) = meas.pos[2];
    meas_mat.at<float>(M_VX) = meas.vec[0];
    meas_mat.at<float>(M_VY) = meas.vec[1];
    meas_mat.at<float>(M_VZ) = meas.vec[2];

    //进行卡尔曼滤波校正（根据新的测量值，矫正滤波器的内部状态）
    m_kf_impl->correct(meas_mat);

    //获取矫正后的S_X, S_Y, S_Z, S_VX, S_VY, S_VZ
    auto pred_pos = Vec3f_t{m_kf_impl->statePost.at<float>(S_X), m_kf_impl->statePost.at<float>(S_Y),
                            m_kf_impl->statePost.at<float>(S_Z)};
    auto pred_vec = Vec3f_t{m_kf_impl->statePost.at<float>(S_VX), m_kf_impl->statePost.at<float>(S_VY),
                            m_kf_impl->statePost.at<float>(S_VZ)};

    //使用EWMA公式，通过预测的位置pred_pos和实际测量的位置m_monentum.pos修正位置信息
    // EWMA公式：m_momentum.pos=α⋅m_momentum.pos+(1−α)⋅pred_pos
    m_momentum.pos = ALPHA.array() * m_momentum.pos.array() + (ONE - ALPHA).array() * pred_pos.array();

    //返回修正后的位置m_momentum.pos和速度pred_vec
    return {m_momentum.pos, pred_vec};
}

/// @brief 进行带校正的目标跟踪
/// @param target_ts 要预测的时间
/// @param meas 测量值信息
/// @return 返回预测时间的目标值信息
Vec3f_t KFPredictor::track_with_correct(double target_ts, PredictorState meas) {
    std::lock_guard<std::mutex> lock(m_mutex);
    update_transition_matrix(target_ts);
    auto pred = this->predict();
    auto cpred = this->correct(std::move(meas));
    last_correct_time_ = target_ts;
    last_measure_time_ = target_ts;

    return cpred.pos;
}

/// @brief 根据卡尔曼滤波进行目标位置预测
/// @param target_ts 预测时间
/// @param with_smooth 是否做3d平滑处理，true表示进行平滑处理，false表示不进行平滑处理
/// @param update_state 预测完成，是否更新状态，true表示更新，false表示不更新
/// @return 返回预测完成的位置信息
Vec3f_t KFPredictor::track_only_pred(double target_ts, bool with_smooth, bool update_state) {
    std::lock_guard<std::mutex> lock(m_mutex);

    //计算一个有效的预测时间
    double valid_target_ts = get_valid_predict_time_length(target_ts);

    //根据新的预测时间，更新卡尔曼滤波器的转移矩阵
    update_transition_matrix(valid_target_ts);

    //根据预测时间，预测目标位置
    std::vector<Vec3f_t> pred_pose;
    PredictorState pred;
    if (update_state) {
        pred = this->predict();
        pred_pose.emplace_back(pred.pos);
        this->correct(pred);
        last_correct_time_ = target_ts;
    } else {
        cv::Mat pred_state = m_kf_impl->transitionMatrix * m_kf_impl->statePost;
        pred_pose.emplace_back(pred_state.at<float>(S_X), pred_state.at<float>(S_Y), pred_state.at<float>(S_Z));
    }

    //如果需要平滑处理，则进行3d平滑处理
    if (with_smooth) {
        predict_smoother_->getFilterHandData(pred_pose);
    }

    //返回预测的结果信息
    return pred_pose[0];
}

/// @brief 根据手部运动动态调整预测时间长度，提高预测的准确性
/// @param target_ts 需要预测的时间戳
/// @return 返回一个经过调整的时间戳
double KFPredictor::get_valid_predict_time_length(double target_ts) {
    //设置时间间隔预设值，0.1表示手完全静止；0.08表示手几乎静止；0.04表示手在运动中
    std::array<double, 3> predict_time_interval_vec = {0.1, 0.08, 0.03};  // 100ms, 80ms, 40ms

    //定义速度阈值，判断手是否静止
    constexpr float static_hand_th_1 = 0.1;
    constexpr float static_hand_th_2 = 0.2;
    int hand_static_state = 0;  // 0表示运动, 1表示几乎静止, 2表示静止

    //计算当前手速（hand_speed = sqrt(S_VX ^ 2 + S_VY ^ 2)，即取手在x和y方向的速度的平方和，再求平方根）
    auto hand_speed = Vec3f_t{m_kf_impl->statePost.at<float>(S_VX), m_kf_impl->statePost.at<float>(S_VY),
                              m_kf_impl->statePost.at<float>(S_VZ)}
                          .norm();

    //根据手速判断当前手的状态是0，1，2
    if (hand_speed < static_hand_th_1) {
        hand_static_state = 2;
    } else if (hand_speed < static_hand_th_2) {
        hand_static_state = 1;
    }

    //以predict_time_interval_vec[hand_static_state]作为预测时间上限，返回合适的预测时间
    double target_timestamp = 0;
    auto predict_interval = (target_ts - last_measure_time_) * predict_length_ratio_;
    predict_interval = std::min(predict_interval, predict_time_interval_vec[hand_static_state]);
    target_timestamp = predict_interval + last_measure_time_;
    return target_timestamp;
}

/// @brief 获取是否在被追踪的状态
/// @return true/false, true表示在被追踪，false表示没有被追踪
bool KFPredictor::get_tracking_status() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return is_tracked;
}

/// @brief 停止追踪
void KFPredictor::stop_tracking() {
    std::lock_guard<std::mutex> lock(m_mutex);
    is_tracked = false;
}

}  // namespace aisdk::algorithm