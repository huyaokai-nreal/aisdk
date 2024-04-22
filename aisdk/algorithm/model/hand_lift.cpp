#include "hand_lift.h"

#include <cstring>

#include "../func/netalgo_utils.h"
#include "../func/pose_solver.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/base/log.h"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {

absl::Status GMLPLiftNet::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
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

    return ret;
}

void GMLPLiftNet::PreProcess(const LiftNetInputs &inputs, const CamInfo &cam_info) {
    int ai = itensor.m_batch * itensor.m_multishape_num;
    int bi = 1;
    if (ai != bi || itensor.m_packed_bybatch == false) {
        return;
    }

    int multi_i, batch_i, height, width, channels, element_byte;
    for (int i = 0; i < bi; i++) {
        multi_i = i / itensor.m_batch;
        batch_i = i % itensor.m_batch;
        if (itensor_format == aisdk::xengine::TensorFormat::CHW) {
            channels = itensor.m_tensors[multi_i].m_dims[0];
            height = itensor.m_tensors[multi_i].m_dims[1];
            width = itensor.m_tensors[multi_i].m_dims[2];
        } else if (itensor_format == aisdk::xengine::TensorFormat::HWC) {
            height = itensor.m_tensors[multi_i].m_dims[0];
            width = itensor.m_tensors[multi_i].m_dims[1];
            channels = itensor.m_tensors[multi_i].m_dims[2];
        }
        element_byte = itensor.m_tensors[multi_i].m_elementbyte;

        int mem_size = height * width * channels * element_byte;
        char *mem = (char *)itensor.m_tensors[multi_i].m_viraddr + batch_i * mem_size;
        // printf("PreProcess: {}, {}, {}, {}\n", height, width,
        // channels, element_byte);
        float *temp = (float *)mem;

        for (int idx = 0; idx < kAlgoKeypointNum; idx++) {
            m_leftcam_x[idx] = (inputs.input_kpt_lcam[idx][0] - cam_info.lcam_intrinsics.at<float>(0, 2)) /
                               cam_info.lcam_intrinsics.at<float>(0, 0);
            m_leftcam_y[idx] = (inputs.input_kpt_lcam[idx][1] - cam_info.lcam_intrinsics.at<float>(1, 2)) /
                               cam_info.lcam_intrinsics.at<float>(1, 1);

            m_rightcam_x[idx] = (inputs.input_kpt_rcam[idx][0] - cam_info.rcam_intrinsics.at<float>(0, 2)) /
                                cam_info.rcam_intrinsics.at<float>(0, 0);
            m_rightcam_y[idx] = (inputs.input_kpt_rcam[idx][1] - cam_info.rcam_intrinsics.at<float>(1, 2)) /
                                cam_info.rcam_intrinsics.at<float>(1, 1);
        }
        std::vector<float> Tmatrix_leftcam = {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1};

        std::vector<float> joint_seq(kAlgoKeypointNum * 3, 0);

        std::vector<float> Tmatrix_lr = {cam_info.cvL_T_cvR(0, 3), cam_info.cvL_T_cvR(1, 3), cam_info.cvL_T_cvR(2, 3),
                                         cam_info.cvL_T_cvR(0, 0), cam_info.cvL_T_cvR(0, 1), cam_info.cvL_T_cvR(0, 2),
                                         cam_info.cvL_T_cvR(1, 0), cam_info.cvL_T_cvR(1, 1), cam_info.cvL_T_cvR(1, 2),
                                         cam_info.cvL_T_cvR(2, 0), cam_info.cvL_T_cvR(2, 1), cam_info.cvL_T_cvR(2, 2)};

        auto buffer_x = temp;
        auto buffer_y = temp + 128;

        // leftcam uv (0-41)
        for (int i = 0; i < kAlgoKeypointNum; i++) {
            buffer_x[i * 2] = m_leftcam_x[i];
            buffer_x[i * 2 + 1] = m_leftcam_y[i];
        }
        // leftcam to leftcam T matrix (42-53)
        for (int i = 0; i < 12; i++) {
            buffer_x[i + 42] = Tmatrix_leftcam[i];
        }
        // lefthand (54)
        buffer_x[54] = inputs.is_left;

        // Sequence data from last timepoint (55-117)
        for (int i = 0; i < 63; i++) {
            buffer_x[i + 55] = joint_seq[i];
        }

        // Zero padding (118-127) do nothing

        // rightcam uv (0-41) + 128
        for (int i = 0; i < kAlgoKeypointNum; i++) {
            buffer_y[i * 2] = m_rightcam_x[i];
            buffer_y[i * 2 + 1] = m_rightcam_y[i];
        }

        // leftcam to rightcam T matrix (42-53) + 128
        for (int i = 0; i < 12; i++) {
            buffer_y[i + 42] = Tmatrix_lr[i];
        }

        // lefthand (54) + 128
        buffer_y[54] = inputs.is_left;

        // Sequence data from last timepoint (55-117) + 128
        for (int i = 0; i < 63; i++) {
            buffer_y[i + 55] = joint_seq[i];
        }
        // Zero padding (118-127) + 128 do nothing
    }
}

void GMLPLiftNet::PostProcess(LiftNetOutputs &outputs, const CamInfo &cam_info) {
    if (otensor.m_packed_bybatch == false) {
        return;
    }

    int _h, _w, _c, element_byte;
    for (int multi_i = 0; multi_i < otensor.m_multishape_num; multi_i++) {
        for (int batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
            if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                _c = otensor.m_tensors[multi_i].m_dims[0];
                _h = otensor.m_tensors[multi_i].m_dims[1];
                _w = otensor.m_tensors[multi_i].m_dims[2];
            } else if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                _h = otensor.m_tensors[multi_i].m_dims[0];
                _w = otensor.m_tensors[multi_i].m_dims[1];
                _c = otensor.m_tensors[multi_i].m_dims[2];
            }
            element_byte = otensor.m_tensors[multi_i].m_elementbyte;
            char *mem = (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * _h * _w * _c * element_byte;
            float *_data = (float *)mem;

            outputs.res3d.resize(21);
            float corruption_cam = 0.5;

            for (int i = 0; i < kAlgoKeypointNum; i++) {
                auto leftZ = _data;
                auto rightZ = _data + 21;

                Vec3f_t leftcam_XYZ{leftZ[i] * m_leftcam_x[i], leftZ[i] * m_leftcam_y[i], leftZ[i]};
                Vec3f_t rightcam_XYZ{rightZ[i] * m_rightcam_x[i], rightZ[i] * m_rightcam_y[i], rightZ[i]};

                Vec3f_t rightcam_root_cv(rightcam_XYZ[0], rightcam_XYZ[1], rightcam_XYZ[2]);
                Vec3f_t rightcam_root_cv_to_left = cam_info.cvL_T_cvR * rightcam_root_cv;
                Vec3f_t rightcam_to_left_XYZ{rightcam_root_cv_to_left.x(), rightcam_root_cv_to_left.y(),
                                             rightcam_root_cv_to_left.z()};
                outputs.res3d[i] = corruption_cam * leftcam_XYZ + (1 - corruption_cam) * rightcam_to_left_XYZ;
            }
        }
    }
}

absl::Status GMLPLiftNet::Inference(const LiftNetInputs &inputs, const CamInfo &cam_info, LiftNetOutputs &outputs) {
    PreProcess(inputs, cam_info);
    auto ret = m_net->RunNet();
    PostProcess(outputs, cam_info);
    return ret;
}

absl::Status GMLPLiftNet3::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session) {
    auto ret = CalculatorBaseNet::Init(algo, model, session);

    if (!ret.ok()) {
        return ret;
    }

    m_leftcam_x.resize(kAlgoKeypointNum);
    m_leftcam_y.resize(kAlgoKeypointNum);
    m_rightcam_x.resize(kAlgoKeypointNum);
    m_rightcam_y.resize(kAlgoKeypointNum);
    mem_left_hand.resize(86);
    mem_right_hand.resize(86);

    return ret;
}

void GMLPLiftNet3::transfer_to_standard_stereo_input() {
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

absl::Status GMLPLiftNet3::SetCameraInfo(const std::shared_ptr<BaseCameraModel> &left_camera,
                                         const std::shared_ptr<BaseCameraModel> &right_camera) {
    left_camera_ = left_camera;
    right_camera_ = right_camera;
    auto [rot_left, rot_right, baseline] =
        get_rotations_for_standard_stereo(right_camera->get_cam_to_world_transform().matrix().cast<double>());
    rot_left_ = rot_left.cast<float>();
    rot_right_ = rot_right.cast<float>();
    baseline_ = baseline;
    return absl::OkStatus();
}

absl::Status GMLPLiftNet3::Inference(const LiftNetInputs &inputs, LiftNetOutputs &outputs) {
    AISDK_LOG_TRACE("[GMLPLiftNet3] Inference");
    PreProcess(inputs);
    auto ret = m_net->RunNet();
    PostProcess(outputs, inputs);
    return ret;
}

void GMLPLiftNet3::PreProcess(const LiftNetInputs &inputs) {
    int index_input = this->m_net->GetInputTensorIndex("feat");
    int input_channels = itensor.m_tensors[index_input].m_dims[0];
    int input_height = itensor.m_tensors[index_input].m_dims[1];
    int input_width = itensor.m_tensors[index_input].m_dims[2];
    int element_byte = itensor.m_tensors[index_input].m_elementbyte;
    int mem_size = input_height * input_width * input_channels * element_byte;
    char *mem = (char *)itensor.m_tensors[index_input].m_viraddr;

    float *temp = (float *)mem;

    auto l_K = left_camera_->get_camera_intrinsics();
    AISDK_LOG_TRACE("[GMLPLiftNet3] lcam cx={}, cy={}, fx={}, fy={}", l_K.cx_, l_K.cy_, l_K.fx_, l_K.fy_);
    auto r_K = right_camera_->get_camera_intrinsics();
    AISDK_LOG_TRACE("[GMLPLiftNet3] rcam cx={}, cy={}, fx={}, fy={}", r_K.cx_, r_K.cy_, r_K.fx_, r_K.fy_);

    for (int idx = 0; idx < kAlgoKeypointNum; idx++) {
        // AISDK_LOG_TRACE("[GMLPLiftNet3] lkpt({}): 0={}, 1={}", idx, inputs.input_kpt_lcam[idx][0],
        //                 inputs.input_kpt_lcam[idx][1]);
        // AISDK_LOG_TRACE("[GMLPLiftNet3] rkpt({}): 0={}, 1={}", idx, inputs.input_kpt_rcam[idx][0],
        //                 inputs.input_kpt_rcam[idx][1]);

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

        // AISDK_LOG_TRACE("[GMLPLiftNet3] buffer_x, {}: {}, {}: {}", i * 2, buffer_x[i * 2], i * 2 + 1,
        //                buffer_x[i * 2 + 1]);
    }
    buffer_x[42] = inputs.is_left;
    for (int i = 0; i < kAlgoKeypointNum; i++) {
        buffer_y[i * 2] = m_rightcam_x[i];
        buffer_y[i * 2 + 1] = m_rightcam_y[i];
        // AISDK_LOG_TRACE("[GMLPLiftNet3] buffer_y, {}: {}, {}: {}", i * 2, buffer_y[i * 2], i * 2 + 1,
        //                 buffer_y[i * 2 + 1]);
    }
    buffer_y[42] = inputs.is_left;

    AISDK_LOG_TRACE("[GMLPLiftNet3] buffer_x[42]={}, buffer_y[42]={}", buffer_x[42], buffer_y[42]);

    int index_mem = this->m_net->GetInputTensorIndex("mem_in");
    int mem_channels = itensor.m_tensors[index_mem].m_dims[0];
    int mem_height = itensor.m_tensors[index_mem].m_dims[1];
    int mem_width = itensor.m_tensors[index_mem].m_dims[2];
    element_byte = itensor.m_tensors[index_mem].m_elementbyte;
    mem_size = mem_height * mem_width * mem_channels;

    char *mem_hand_c = (char *)itensor.m_tensors[index_mem].m_viraddr;
    float *mem_hand = (float *)mem_hand_c;
    if (inputs.is_left != 0. && (inputs.timestamp - last_left_time) < reset_mem_time) {
        memcpy(mem_hand, mem_right_hand.data(), mem_size);
    } else if (inputs.is_left == 0. && (inputs.timestamp - last_right_time) < reset_mem_time) {
        memcpy(mem_hand, mem_right_hand.data(), mem_size);
    } else {
        memset(mem_hand, 0, mem_size);
    }
}

void GMLPLiftNet3::PostProcess(LiftNetOutputs &outputs, const LiftNetInputs &inputs) {
    int index_output = this->m_net->GetOutputTensorIndex("kpt");

    char *mem = (char *)otensor.m_tensors[index_output].m_viraddr;
    float *_data = (float *)mem;

    outputs.res3d.resize(21);
    float corruption_cam = 0.5;

    for (int i = 0; i < kAlgoKeypointNum; i++) {
        auto leftZ = _data;
        auto rightZ = _data + 21;

        auto left_depth = leftZ[i] * baseline_;
        auto right_depth = rightZ[i] * baseline_;

        Eigen::Vector3f leftcam_XYZ;
        Eigen::Vector3f rightcam_XYZ;

        leftcam_XYZ << left_depth * m_leftcam_x[i], left_depth * m_leftcam_y[i], left_depth;
        rightcam_XYZ << right_depth * m_rightcam_x[i], right_depth * m_rightcam_y[i], right_depth;

        leftcam_XYZ.noalias() = rot_left_.transpose() * leftcam_XYZ;
        rightcam_XYZ.noalias() = rot_right_.transpose() * rightcam_XYZ;

        Eigen::Vector3f rightcam_to_left_XYZ = right_camera_->get_cam_to_world_transform() * rightcam_XYZ;
        Eigen::Vector3f final_kpt = corruption_cam * leftcam_XYZ + (1 - corruption_cam) * rightcam_to_left_XYZ;

        outputs.res3d[i] = Vec3f_t{final_kpt.x(), final_kpt.y(), final_kpt.z()};
    }

    int index_mem = this->m_net->GetOutputTensorIndex("mem_out");

    int mem_channels = otensor.m_tensors[index_mem].m_dims[0];
    int mem_height = otensor.m_tensors[index_mem].m_dims[1];
    int mem_width = otensor.m_tensors[index_mem].m_dims[2];

    int mem_size = mem_height * mem_width * mem_channels;
    float *mem_hand = (float *)otensor.m_tensors[index_mem].m_viraddr;

    if (inputs.is_left != 0.) {
        memcpy(mem_left_hand.data(), mem_hand, mem_size);
        last_left_time = inputs.timestamp;
    } else {
        memcpy(mem_right_hand.data(), mem_hand, mem_size);
        last_right_time = inputs.timestamp;
    }
}

// liftnimble

aisdk::xengine::Status GMLPLiftNimble::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                            aisdk::xengine::SessionConfig &session) {
    auto ret = CalculatorBaseNet::Init(algo, model, session);

    if (ret != aisdk::xengine::Status::SUCCESS) {
        return ret;
    }

    m_leftcam_x.resize(kKeypointNum);
    m_leftcam_y.resize(kKeypointNum);
    m_rightcam_x.resize(kKeypointNum);
    m_rightcam_y.resize(kKeypointNum);
    mem_left_hand.resize(86);
    mem_right_hand.resize(86);

    return aisdk::xengine::Status::SUCCESS;
}

void GMLPLiftNimble::transfer_to_standard_stereo_input() {
    using KptMatrix = Eigen::Matrix<float, 21, 3>;
    KptMatrix left_kpt_homo = KptMatrix::Ones();
    KptMatrix right_kpt_homo = KptMatrix::Ones();
    for (size_t i = 0; i < kKeypointNum; i++) {
        left_kpt_homo(i, 0) = m_leftcam_x[i];
        left_kpt_homo(i, 1) = m_leftcam_y[i];
        right_kpt_homo(i, 0) = m_rightcam_x[i];
        right_kpt_homo(i, 1) = m_rightcam_y[i];
    }
    left_kpt_homo.noalias() = (rot_left_ * left_kpt_homo.transpose()).transpose();
    right_kpt_homo.noalias() = (rot_right_ * right_kpt_homo.transpose()).transpose();
    left_kpt_homo = left_kpt_homo.array().colwise() / left_kpt_homo.array().col(2);
    right_kpt_homo = right_kpt_homo.array().colwise() / right_kpt_homo.array().col(2);
    for (size_t i = 0; i < kKeypointNum; i++) {
        m_leftcam_x[i] = left_kpt_homo(i, 0);
        m_leftcam_y[i] = left_kpt_homo(i, 1);
        m_rightcam_x[i] = right_kpt_homo(i, 0);
        m_rightcam_y[i] = right_kpt_homo(i, 1);
    }
}

aisdk::xengine::Status GMLPLiftNimble::SetCameraInfo(const std::shared_ptr<BaseCameraModel> &left_camera,
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
    return aisdk::xengine::Status::SUCCESS;
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
    AISDK_LOG_TRACE("[GMLPLiftNimble] lcam cx={}, cy={}, fx={}, fy={}", l_K.cx_, l_K.cy_, l_K.fx_, l_K.fy_);
    auto r_K = right_camera_->get_camera_intrinsics();
    AISDK_LOG_TRACE("[GMLPLiftNimble] rcam cx={}, cy={}, fx={}, fy={}", r_K.cx_, r_K.cy_, r_K.fx_, r_K.fy_);

    for (int idx = 0; idx < kKeypointNum; idx++) {
        AISDK_LOG_TRACE("[GMLPLiftNimble] lkpt({}): 0={}, 1={}", idx, inputs.input_kpt_lcam[idx][0],
                        inputs.input_kpt_lcam[idx][1]);
        AISDK_LOG_TRACE("[GMLPLiftNimble] rkpt({}): 0={}, 1={}", idx, inputs.input_kpt_lcam[idx][0],
                        inputs.input_kpt_lcam[idx][1]);

        m_leftcam_x[idx] = (inputs.input_kpt_lcam[idx][0] - l_K.cx_) / l_K.fx_;
        m_leftcam_y[idx] = (inputs.input_kpt_lcam[idx][1] - l_K.cy_) / l_K.fy_;
        m_rightcam_x[idx] = (inputs.input_kpt_rcam[idx][0] - r_K.cx_) / r_K.fx_;
        m_rightcam_y[idx] = (inputs.input_kpt_rcam[idx][1] - r_K.cy_) / r_K.fy_;
    }

    transfer_to_standard_stereo_input();

    auto buffer_x = temp;
    auto buffer_y = temp + 43;

    for (int i = 0; i < kKeypointNum; i++) {
        buffer_x[i * 2] = m_leftcam_x[i];
        buffer_x[i * 2 + 1] = m_leftcam_y[i];

        AISDK_LOG_TRACE("[GMLPLiftNimble] buffer_x, {}: {}, {}: {}", i * 2, buffer_x[i * 2], i * 2 + 1,
                        buffer_x[i * 2 + 1]);
    }
    buffer_x[42] = inputs.is_left;
    for (int i = 0; i < kKeypointNum; i++) {
        buffer_y[i * 2] = m_rightcam_x[i];
        buffer_y[i * 2 + 1] = m_rightcam_y[i];
        AISDK_LOG_TRACE("[GMLPLiftNimble] buffer_y, {}: {}, {}: {}", i * 2, buffer_y[i * 2], i * 2 + 1,
                        buffer_y[i * 2 + 1]);
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

    for (int i = 0; i < mem_size; i++) {
        AISDK_LOG_TRACE("[GMLPLiftNimble] mem_hand[{}] = {}", i, mem_hand[i]);
    }
}

void GMLPLiftNimble::PostProcess(LiftNimbleNetOutputs &outputs, const LiftNetInputs &inputs) {
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

    // kpt
    int index_output = this->m_net->GetOutputTensorIndex("kpt");
    float *kpt_ptr = (float *)otensor.m_tensors[index_output].m_viraddr;
    Eigen::Map<Eigen::Matrix<float, 21, 3, Eigen::RowMajor>> local_kpt(kpt_ptr);
    Eigen::Matrix<float, 21, 3> global_kpt =
        ((global_rotation * local_kpt.transpose()).transpose().rowwise() + global_translation.transpose()) *
        baseline_scale_ / standard_baseline_;
    outputs.res3d.resize(21);
    for (int i = 0; i < kKeypointNum; i++) {
        outputs.res3d[i] = cv::Vec3f{global_kpt(i, 0), global_kpt(i, 1), global_kpt(i, 2)};
    }

    // get local hand pose
    int index_matrix = this->m_net->GetOutputTensorIndex("matrix");
    float *matrix_ptr = (float *)otensor.m_tensors[index_matrix].m_viraddr;
    Eigen::Map<Eigen::Matrix<float, 19, 9, Eigen::RowMajor>> local_matrix(matrix_ptr);
    Eigen::Matrix<float, 1, 9> reshaped_global_rotation;
    reshaped_global_rotation << global_rotation(0, 0), global_rotation(0, 1), global_rotation(0, 2),
        global_rotation(1, 0), global_rotation(1, 1), global_rotation(1, 2), global_rotation(2, 0),
        global_rotation(2, 1), global_rotation(2, 2);
    outputs.angle.resize(20);
    outputs.angle[0] = reshaped_global_rotation;
    for (int i = 1; i < NimblePoseNum; i++) {
        outputs.angle[i] = local_matrix.block<1, 9>(i - 1, 0);
    }

    // get shape and trans
    int index_trans = this->m_net->GetOutputTensorIndex("trans");
    float *trans_ptr = (float *)otensor.m_tensors[index_trans].m_viraddr;
    outputs.trans.resize(1);
    outputs.trans[0] = cv::Vec3f(trans_ptr[0], trans_ptr[1], trans_ptr[2]);
    int index_shape = this->m_net->GetOutputTensorIndex("shape");
    float *shape_ptr = (float *)otensor.m_tensors[index_shape].m_viraddr;
    outputs.shape.resize(1);
    outputs.shape[0] = shape_ptr[0];

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
}

aisdk::xengine::Status GMLPLiftNimble::Inference(const LiftNetInputs &inputs, LiftNimbleNetOutputs &outputs) {
    AISDK_LOG_TRACE("[GMLPLiftNimble] Inference");
    PreProcess(inputs);
    aisdk::xengine::Status ret = m_net->RunNet();
    AISDK_LOG_INFO("[GMLPLiftNimble] run GMLPLiftNimble success");
    PostProcess(outputs, inputs);
    AISDK_LOG_INFO("[GMLPLiftNimble] run GMLPLiftNimble PostProcess success");
    return ret;
}

}  // namespace aisdk::algorithm
