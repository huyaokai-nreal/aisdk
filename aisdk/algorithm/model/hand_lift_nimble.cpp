#include "hand_lift_nimble.h"

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <fmt/format.h>

#include <cstring>
#include <vector>

#include "../func/netalgo_utils.h"
#include "../func/pose_solver.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/func/hand_nimble.h"
#include "aisdk/base/log.h"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {

absl::Status GMLPLiftNimble::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                  aisdk::xengine::SessionConfig &session) {
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }

    itensor_format = checkshapeformat(model.vendor_type, itensor.m_tensors[0].m_rank);
    otensor_format = checkshapeformat(model.vendor_type, otensor.m_tensors[0].m_rank);
    m_leftcam_x.resize(kAlgoKeypointNum);
    m_leftcam_y.resize(kAlgoKeypointNum);
    m_rightcam_x.resize(kAlgoKeypointNum);
    m_rightcam_y.resize(kAlgoKeypointNum);
    mem_left_hand.resize(86);
    mem_right_hand.resize(86);
    return ret;
}

void GMLPLiftNimble::transfer_to_standard_stereo_input() {
    using KptMatrix = Eigen::Matrix<float, 21, 3>;
    KptMatrix left_kpt_homo = KptMatrix::Ones();
    KptMatrix right_kpt_homo = KptMatrix::Ones();
    for (size_t i = 0; i < kAlgoKeypointNum; i++) {
        left_kpt_homo(i, 0) = m_leftcam_x[i];
        left_kpt_homo(i, 1) = m_leftcam_y[i];
        right_kpt_homo(i, 0) = m_rightcam_x[i];
        right_kpt_homo(i, 1) = m_rightcam_y[i];
    }
    left_kpt_homo.noalias() = (rot_left_ * left_kpt_homo.transpose()).transpose();
    right_kpt_homo.noalias() = (rot_right_ * right_kpt_homo.transpose()).transpose();
    left_kpt_homo = left_kpt_homo.array().colwise() / left_kpt_homo.array().col(2);
    right_kpt_homo = right_kpt_homo.array().colwise() / right_kpt_homo.array().col(2);
    for (size_t i = 0; i < kAlgoKeypointNum; i++) {
        m_leftcam_x[i] = left_kpt_homo(i, 0);
        m_leftcam_y[i] = left_kpt_homo(i, 1);
        m_rightcam_x[i] = right_kpt_homo(i, 0);
        m_rightcam_y[i] = right_kpt_homo(i, 1);
    }
}

absl::Status GMLPLiftNimble::SetCameraInfo(const std::shared_ptr<BaseCameraModel> &left_camera,
                                           const std::shared_ptr<BaseCameraModel> &right_camera) {
    left_camera_ = left_camera;
    right_camera_ = right_camera;
    if (!init_camera_info_) {
        auto [rot_left, rot_right, baseline] =
            get_rotations_for_standard_stereo(right_camera->get_cam_to_world_transform().matrix().cast<double>());
        rot_left_ = rot_left.cast<float>();
        rot_right_ = rot_right.cast<float>();
        init_camera_info_ = true;
        baseline_scale_ = baseline;
    }
    return absl::OkStatus();
}

void GMLPLiftNimble::PreProcess(const LiftNetInputs &inputs) {
    int index_input = this->m_net->GetInputTensorIndex("feat");
    int input_channels = itensor.m_tensors[index_input].m_dims[0];
    int input_height = itensor.m_tensors[index_input].m_dims[1];
    int input_width = itensor.m_tensors[index_input].m_dims[2];
    int element_byte = itensor.m_tensors[index_input].m_elementbyte;
    int mem_size = input_height * input_width * input_channels * element_byte;
    char *mem = (char *)itensor.m_tensors[index_input].m_viraddr;

    float *temp = (float *)mem;

    auto l_K = left_camera_->get_camera_intrinsics();
    auto r_K = right_camera_->get_camera_intrinsics();
    for (int idx = 0; idx < kAlgoKeypointNum; idx++) {
        m_leftcam_x[idx] = (inputs.input_kpt_lcam[idx][0] - l_K.cx_) / l_K.fx_;
        m_leftcam_y[idx] = (inputs.input_kpt_lcam[idx][1] - l_K.cy_) / l_K.fy_;
        m_rightcam_x[idx] = (inputs.input_kpt_rcam[idx][0] - r_K.cx_) / r_K.fx_;
        m_rightcam_y[idx] = (inputs.input_kpt_rcam[idx][1] - r_K.cy_) / r_K.fy_;
    }

    transfer_to_standard_stereo_input();

    auto buffer_x = temp;
    auto buffer_y = temp + 43;

    for (int i = 0; i < kAlgoKeypointNum; i++) {
        buffer_x[i * 2] = m_leftcam_x[i];
        buffer_x[i * 2 + 1] = m_leftcam_y[i];
    }
    buffer_x[42] = inputs.is_left;
    for (int i = 0; i < kAlgoKeypointNum; i++) {
        buffer_y[i * 2] = m_rightcam_x[i];
        buffer_y[i * 2 + 1] = m_rightcam_y[i];
    }
    buffer_y[42] = inputs.is_left;

    AISDK_LOG_TRACE("[GMLPLiftNimble] buffer_x[42]={}, buffer_y[42]={}", buffer_x[42], buffer_y[42]);

    int index_mem = this->m_net->GetInputTensorIndex("mem_in");
    int mem_channels = itensor.m_tensors[index_mem].m_dims[0];
    int mem_height = itensor.m_tensors[index_mem].m_dims[1];
    int mem_width = itensor.m_tensors[index_mem].m_dims[2];

    element_byte = itensor.m_tensors[index_mem].m_elementbyte;
    mem_size = mem_height * mem_width * mem_channels;

    char *mem_hand_c = (char *)itensor.m_tensors[index_mem].m_viraddr;
    float *mem_hand = (float *)mem_hand_c;

    if (inputs.is_left != 0. && inputs.timestamp - last_left_time < reset_mem_time) {
        for (int i = 0; i < mem_size; i++) {
            mem_hand[i] = mem_left_hand[i];
        }
    } else if (inputs.is_left == 0. && inputs.timestamp - last_right_time < reset_mem_time) {
        for (int i = 0; i < mem_size; i++) {
            mem_hand[i] = mem_right_hand[i];
        }
    } else {
        AISDK_LOG_TRACE("[GMLPLiftNimble] reset liftnetv3 mem");
        for (int i = 0; i < mem_size; i++) {
            mem_hand[i] = 0;
        }
    }
}

void GMLPLiftNimble::PostProcess(const LiftNetInputs &inputs, LiftNetOutputs &outputs) {
    // get global transform
    int index_svd_pt = this->m_net->GetOutputTensorIndex("svd_pt");
    float *svd_pt_ptr = (float *)otensor.m_tensors[index_svd_pt].m_viraddr;
    Eigen::Map<Eigen::Matrix<float, 7, 3, Eigen::RowMajor>> svd_pt(svd_pt_ptr);
    Eigen::Matrix<float, 7, 3> svd_src_pt;
    svd_src_pt << 0., 0., 0., 0.1, 0., 0., 0., 0.1, 0., 0., 0., 0.1, -0.07071068, -0.07071068, 0., -0.07071068, 0.,
        -0.07071068, 0., -0.07071068, -0.07071068;
    Eigen::Isometry3f global_hand_pose = get_transform_with_svd(svd_src_pt, svd_pt);
    Eigen::Matrix3f global_rotation = global_hand_pose.rotation();
    Eigen::Vector3f global_translation = global_hand_pose.translation();
    global_translation.noalias() = rot_left_.transpose() * global_translation;
    global_rotation.noalias() = rot_left_.transpose() * global_rotation;

    // right to left
    if (inputs.is_left != 0.) {
        Eigen::Matrix3f flip_x;
        flip_x << -1, 0, 0, 0, 1, 0, 0, 0, 1;
        global_rotation.noalias() = global_rotation * flip_x;
    }
    // shape
    int index_shape = this->m_net->GetOutputTensorIndex("shape");
    float shape_param = *((float *)otensor.m_tensors[index_shape].m_viraddr);
    // angle
    // kpt
    int index_angle = this->m_net->GetOutputTensorIndex("angle");
    float *angle_ptr = (float *)otensor.m_tensors[index_angle].m_viraddr;
    std::vector<float> local_angles(angle_ptr, angle_ptr + 171);
    auto local_kpt = decode_hand_joints(shape_param, local_angles);
    Eigen::Matrix<float, 21, 3> global_kpt =
        ((global_rotation * local_kpt.transpose()).transpose().rowwise() + global_translation.transpose()) *
        baseline_scale_ / standard_baseline_;
    outputs.res3d.resize(21);
    for (int i = 0; i < kAlgoKeypointNum; i++) {
        outputs.res3d[i] = Vec3f_t{global_kpt(i, 0), global_kpt(i, 1), global_kpt(i, 2)};
    }
    AISDK_LOG_TRACE("[GMLPLiftNimble] run GMLPLiftNimble infer kpt success");

    int index_mem = this->m_net->GetOutputTensorIndex("mem_out");

    int mem_channels = otensor.m_tensors[index_mem].m_dims[0];
    int mem_height = otensor.m_tensors[index_mem].m_dims[1];
    int mem_width = otensor.m_tensors[index_mem].m_dims[2];

    int mem_size = mem_height * mem_width * mem_channels;
    float *mem_hand = (float *)otensor.m_tensors[index_mem].m_viraddr;

    if (inputs.is_left != 0.) {
        for (int i = 0; i < mem_size; i++) {
            mem_left_hand[i] = mem_hand[i];
        }
        last_left_time = inputs.timestamp;
    } else {
        for (int i = 0; i < mem_size; i++) {
            mem_right_hand[i] = mem_hand[i];
        }
        last_right_time = inputs.timestamp;
    }
    AISDK_LOG_TRACE("[GMLPLiftNimble] run GMLPLiftNimble infer mem success");
}

absl::StatusOr<LiftNetOutputs> GMLPLiftNimble::Inference(const LiftNetInputs &inputs) {
    AISDK_LOG_TRACE("[GMLPLiftNimble] Inference");
    PreProcess(inputs);
    auto ret = m_net->RunNet();
    if (ret.ok()) {
        AISDK_LOG_TRACE("[GMLPLiftNimble] run GMLPLiftNimble infer success");
        LiftNetOutputs outputs;
        PostProcess(inputs, outputs);
        AISDK_LOG_TRACE("[GMLPLiftNimble] run GMLPLiftNimble PostProcess success");
        return outputs;
    }
    AISDK_LOG_WARN(ret.message());
    return absl::UnavailableError(fmt::format("failed to get liftnimble infer resut with {}", ret.message()));
}

}  // namespace aisdk::algorithm
