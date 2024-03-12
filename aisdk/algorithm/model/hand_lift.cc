#include "hand_lift.h"

#include "../func/netalgo_utils.h"
#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"

namespace aisdk::algorithm {

aisdk::xengine::Status GMLPLiftNet::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                         aisdk::xengine::SessionConfig &session) {
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (ret != aisdk::xengine::Status::SUCCESS) {
        return ret;
    }

    // 简单实现
    {
        itensor_format = aisdk::xengine::TensorFormat::CHW;
        // 这里应该从打包传入dims
        std::vector<uint32_t> iexpect{256, 1, 1};
        if (false == checkshapeformat(itensor.m_tensors[0].m_rank, itensor.m_tensors[0].m_dims, iexpect)) {
            itensor_format = aisdk::xengine::TensorFormat::HWC;
        }
    }

    {
        otensor_format = aisdk::xengine::TensorFormat::CHW;
        // 这里应该从打包传入dims
        std::vector<uint32_t> oexpect{42, 1, 1};
        if (false == checkshapeformat(otensor.m_tensors[0].m_rank, otensor.m_tensors[0].m_dims, oexpect)) {
            otensor_format = aisdk::xengine::TensorFormat::HWC;
        }
    }

    m_leftcam_x.resize(KPT_NUM);
    m_leftcam_y.resize(KPT_NUM);
    m_rightcam_x.resize(KPT_NUM);
    m_rightcam_y.resize(KPT_NUM);

    return aisdk::xengine::Status::SUCCESS;
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

        for (int idx = 0; idx < KPT_NUM; idx++) {
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

        std::vector<float> joint_seq(KPT_NUM * 3, 0);

        std::vector<float> Tmatrix_lr = {cam_info.cvL_T_cvR(0, 3), cam_info.cvL_T_cvR(1, 3), cam_info.cvL_T_cvR(2, 3),
                                         cam_info.cvL_T_cvR(0, 0), cam_info.cvL_T_cvR(0, 1), cam_info.cvL_T_cvR(0, 2),
                                         cam_info.cvL_T_cvR(1, 0), cam_info.cvL_T_cvR(1, 1), cam_info.cvL_T_cvR(1, 2),
                                         cam_info.cvL_T_cvR(2, 0), cam_info.cvL_T_cvR(2, 1), cam_info.cvL_T_cvR(2, 2)};

        auto buffer_x = temp;
        auto buffer_y = temp + 128;

        // leftcam uv (0-41)
        for (int i = 0; i < KPT_NUM; i++) {
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
        for (int i = 0; i < KPT_NUM; i++) {
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

            for (int i = 0; i < KPT_NUM; i++) {
                auto leftZ = _data;
                auto rightZ = _data + 21;

                cv::Vec3f leftcam_XYZ{leftZ[i] * m_leftcam_x[i], leftZ[i] * m_leftcam_y[i], leftZ[i]};
                cv::Vec3f rightcam_XYZ{rightZ[i] * m_rightcam_x[i], rightZ[i] * m_rightcam_y[i], rightZ[i]};

                Eigen::Vector3f rightcam_root_cv(rightcam_XYZ[0], rightcam_XYZ[1], rightcam_XYZ[2]);
                Eigen::Vector3f rightcam_root_cv_to_left = cam_info.cvL_T_cvR * rightcam_root_cv;
                cv::Vec3f rightcam_to_left_XYZ{rightcam_root_cv_to_left.x(), rightcam_root_cv_to_left.y(),
                                               rightcam_root_cv_to_left.z()};
                outputs.res3d[i] = corruption_cam * leftcam_XYZ + (1 - corruption_cam) * rightcam_to_left_XYZ;
            }
        }
    }
}

aisdk::xengine::Status GMLPLiftNet::Inference(const LiftNetInputs &inputs, const CamInfo &cam_info,
                                              LiftNetOutputs &outputs) {
    PreProcess(inputs, cam_info);
    aisdk::xengine::Status ret = m_net->RunNet();
    PostProcess(outputs, cam_info);
    return ret;
}

aisdk::xengine::Status SeqGMLPLiftNet::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                            aisdk::xengine::SessionConfig &session) {
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (ret != aisdk::xengine::Status::SUCCESS) {
        return ret;
    }

    // 简单实现
    {
        itensor_format = aisdk::xengine::TensorFormat::CHW;
        // 这里应该从打包传入dims
        std::vector<uint32_t> iexpect{110, 1, 1};
        if (false == checkshapeformat(itensor.m_tensors[0].m_rank, itensor.m_tensors[0].m_dims, iexpect)) {
            itensor_format = aisdk::xengine::TensorFormat::HWC;
        }
    }

    {
        otensor_format = aisdk::xengine::TensorFormat::CHW;
        // 这里应该从打包传入dims
        std::vector<uint32_t> oexpect{42, 1, 1};
        if (false == checkshapeformat(otensor.m_tensors[0].m_rank, otensor.m_tensors[0].m_dims, oexpect)) {
            otensor_format = aisdk::xengine::TensorFormat::HWC;
        }
    }

    m_leftcam_x.resize(KPT_NUM);
    m_leftcam_y.resize(KPT_NUM);
    m_rightcam_x.resize(KPT_NUM);
    m_rightcam_y.resize(KPT_NUM);

    return aisdk::xengine::Status::SUCCESS;
}

void SeqGMLPLiftNet::PreProcess(const LiftNetInputs &inputs, const CamInfo &cam_info) {
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

        for (int idx = 0; idx < KPT_NUM; idx++) {
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

        std::vector<float> Tmatrix_lr = {cam_info.cvL_T_cvR(0, 3), cam_info.cvL_T_cvR(1, 3), cam_info.cvL_T_cvR(2, 3),
                                         cam_info.cvL_T_cvR(0, 0), cam_info.cvL_T_cvR(0, 1), cam_info.cvL_T_cvR(0, 2),
                                         cam_info.cvL_T_cvR(1, 0), cam_info.cvL_T_cvR(1, 1), cam_info.cvL_T_cvR(1, 2),
                                         cam_info.cvL_T_cvR(2, 0), cam_info.cvL_T_cvR(2, 1), cam_info.cvL_T_cvR(2, 2)};

        auto buffer_x = temp;
        auto buffer_y = temp + 55;

        // leftcam uv (0-41)
        for (int i = 0; i < KPT_NUM; i++) {
            buffer_x[i * 2] = m_leftcam_x[i];
            buffer_x[i * 2 + 1] = m_leftcam_y[i];
        }
        // leftcam to leftcam T matrix (42-53)
        for (int i = 0; i < 12; i++) {
            buffer_x[i + 42] = Tmatrix_leftcam[i];
        }
        // lefthand (54)
        buffer_x[54] = inputs.is_left;

        // rightcam uv (0-41) + 55
        for (int i = 0; i < KPT_NUM; i++) {
            buffer_y[i * 2] = m_rightcam_x[i];
            buffer_y[i * 2 + 1] = m_rightcam_y[i];
        }

        // leftcam to rightcam T matrix (42-53) + 55
        for (int i = 0; i < 12; i++) {
            buffer_y[i + 42] = Tmatrix_lr[i];
        }

        // lefthand (54) + 55
        buffer_y[54] = inputs.is_left;
    }
}

void SeqGMLPLiftNet::PostProcess(LiftNetOutputs &outputs, const CamInfo &cam_info) {
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

            for (int i = 0; i < KPT_NUM; i++) {
                auto leftZ = _data;
                auto rightZ = _data + 21;

                cv::Vec3f leftcam_XYZ{leftZ[i] * m_leftcam_x[i], leftZ[i] * m_leftcam_y[i], leftZ[i]};
                cv::Vec3f rightcam_XYZ{rightZ[i] * m_rightcam_x[i], rightZ[i] * m_rightcam_y[i], rightZ[i]};

                Eigen::Vector3f rightcam_root_cv(rightcam_XYZ[0], rightcam_XYZ[1], rightcam_XYZ[2]);
                Eigen::Vector3f rightcam_root_cv_to_left = cam_info.cvL_T_cvR * rightcam_root_cv;
                cv::Vec3f rightcam_to_left_XYZ{rightcam_root_cv_to_left.x(), rightcam_root_cv_to_left.y(),
                                               rightcam_root_cv_to_left.z()};
                outputs.res3d[i] = corruption_cam * leftcam_XYZ + (1 - corruption_cam) * rightcam_to_left_XYZ;
            }
        }
    }
}

aisdk::xengine::Status SeqGMLPLiftNet::Inference(const LiftNetInputs &inputs, const CamInfo &cam_info,
                                                 LiftNetOutputs &outputs) {
    PreProcess(inputs, cam_info);
    aisdk::xengine::Status ret = m_net->RunNet();
    PostProcess(outputs, cam_info);
    return ret;
}

aisdk::xengine::Status GMLPLiftNet3::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                          aisdk::xengine::SessionConfig &session) {
    auto ret = CalculatorBaseNet::Init(algo, model, session);

    if (ret != aisdk::xengine::Status::SUCCESS) {
        return ret;
    }

    m_leftcam_x.resize(KPT_NUM);
    m_leftcam_y.resize(KPT_NUM);
    m_rightcam_x.resize(KPT_NUM);
    m_rightcam_y.resize(KPT_NUM);
    mem_left_hand.resize(86);
    mem_right_hand.resize(86);

    return aisdk::xengine::Status::SUCCESS;
}

void GMLPLiftNet3::transfer_to_standard_stereo_input() {
    using KptMatrix = Eigen::Matrix<float, 21, 3>;
    KptMatrix left_kpt_homo = KptMatrix::Ones();
    KptMatrix right_kpt_homo = KptMatrix::Ones();
    for (size_t i = 0; i < KPT_NUM; i++) {
        left_kpt_homo(i, 0) = m_leftcam_x[i];
        left_kpt_homo(i, 1) = m_leftcam_y[i];
        right_kpt_homo(i, 0) = m_rightcam_x[i];
        right_kpt_homo(i, 1) = m_rightcam_y[i];
    }
    left_kpt_homo.noalias() = (rot_left_ * left_kpt_homo.transpose()).transpose();
    right_kpt_homo.noalias() = (rot_right_ * right_kpt_homo.transpose()).transpose();
    left_kpt_homo = left_kpt_homo.array().colwise() / left_kpt_homo.array().col(2);
    right_kpt_homo = right_kpt_homo.array().colwise() / right_kpt_homo.array().col(2);
    for (size_t i = 0; i < KPT_NUM; i++) {
        m_leftcam_x[i] = left_kpt_homo(i, 0);
        m_leftcam_y[i] = left_kpt_homo(i, 1);
        m_rightcam_x[i] = right_kpt_homo(i, 0);
        m_rightcam_y[i] = right_kpt_homo(i, 1);
    }
}

aisdk::xengine::Status GMLPLiftNet3::SetCamInfo(const CamInfo &cam_info) {
    m_cam_info = cam_info;
    if (!init_camera_info_) {
        auto [rot_left, rot_right, baseline] = get_rotations_for_standard_stereo(cam_info.cvL_T_cvR.matrix());
        rot_left_ = rot_left;
        rot_right_ = rot_right;
        baseline_scale_ = baseline / standard_baseline_;
        init_camera_info_ = true;

        std::cout << "cam_info lcam intr:" << cam_info.lcam_intrinsics << std::endl;
        std::cout << "rot_left:" << rot_left_ << std::endl;
    }
    return aisdk::xengine::Status::SUCCESS;
}

aisdk::xengine::Status GMLPLiftNet3::Inference(const LiftNetInputs &inputs, const CamInfo &cam_info,
                                               LiftNetOutputs &outputs) {
    AISDK_LOG_TRACE("[GMLPLiftNet3] Inference");
    PreProcess(inputs, cam_info);
    aisdk::xengine::Status ret = m_net->RunNet();
    PostProcess(outputs, cam_info, inputs);  // TIPS: v3这里改了PostProcess接口是因为develop分支这里面用iodata混用,
                                             // 从output中取了输入信息, 所以要用到inputs
    return ret;
}

void GMLPLiftNet3::PreProcess(const LiftNetInputs &inputs, const CamInfo &cam_info) {
    int index_input = this->m_net->GetInputTensorIndex("feat");
    int input_channels = itensor.m_tensors[index_input].m_dims[0];
    int input_height = itensor.m_tensors[index_input].m_dims[1];
    int input_width = itensor.m_tensors[index_input].m_dims[2];
    int element_byte = itensor.m_tensors[index_input].m_elementbyte;
    int mem_size = input_height * input_width * input_channels * element_byte;
    char *mem = (char *)itensor.m_tensors[index_input].m_viraddr;

    float *temp = (float *)mem;

    auto l_K = m_cam_info.lcam_intrinsics;
    AISDK_LOG_TRACE("[GMLPLiftNet3] lcam 00={}, 02={}, 11={}, 12={}", l_K.at<float>(0, 0), l_K.at<float>(0, 2),
                    l_K.at<float>(1, 1), l_K.at<float>(1, 2));
    auto r_K = m_cam_info.rcam_intrinsics;
    AISDK_LOG_TRACE("[GMLPLiftNet3] rcam 00={}, 02={}, 11={}, 12={}", r_K.at<float>(0, 0), r_K.at<float>(0, 2),
                    r_K.at<float>(1, 1), r_K.at<float>(1, 2));

    for (int idx = 0; idx < KPT_NUM; idx++) {
        AISDK_LOG_TRACE("[GMLPLiftNet3] lkpt({}): 0={}, 1={}", idx, inputs.input_kpt_lcam[idx][0],
                        inputs.input_kpt_lcam[idx][1]);
        AISDK_LOG_TRACE("[GMLPLiftNet3] rkpt({}): 0={}, 1={}", idx, inputs.input_kpt_lcam[idx][0],
                        inputs.input_kpt_lcam[idx][1]);

        m_leftcam_x[idx] = (inputs.input_kpt_lcam[idx][0] - m_cam_info.lcam_intrinsics.at<float>(0, 2)) /
                           m_cam_info.lcam_intrinsics.at<float>(0, 0);
        m_leftcam_y[idx] = (inputs.input_kpt_lcam[idx][1] - m_cam_info.lcam_intrinsics.at<float>(1, 2)) /
                           m_cam_info.lcam_intrinsics.at<float>(1, 1);

        m_rightcam_x[idx] = (inputs.input_kpt_rcam[idx][0] - m_cam_info.rcam_intrinsics.at<float>(0, 2)) /
                            m_cam_info.rcam_intrinsics.at<float>(0, 0);
        m_rightcam_y[idx] = (inputs.input_kpt_rcam[idx][1] - m_cam_info.rcam_intrinsics.at<float>(1, 2)) /
                            m_cam_info.rcam_intrinsics.at<float>(1, 1);
    }

    transfer_to_standard_stereo_input();

    auto buffer_x = temp;
    auto buffer_y = temp + 43;

    for (int i = 0; i < KPT_NUM; i++) {
        buffer_x[i * 2] = m_leftcam_x[i];
        buffer_x[i * 2 + 1] = m_leftcam_y[i];

        AISDK_LOG_TRACE("[GMLPLiftNet3] buffer_x, {}: {}, {}: {}", i * 2, buffer_x[i * 2], i * 2 + 1,
                        buffer_x[i * 2 + 1]);
    }
    buffer_x[42] = inputs.is_left;
    for (int i = 0; i < KPT_NUM; i++) {
        buffer_y[i * 2] = m_rightcam_x[i];
        buffer_y[i * 2 + 1] = m_rightcam_y[i];
        AISDK_LOG_TRACE("[GMLPLiftNet3] buffer_y, {}: {}, {}: {}", i * 2, buffer_y[i * 2], i * 2 + 1,
                        buffer_y[i * 2 + 1]);
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

    if (inputs.is_left != 0. && float(inputs.timestamp[0] / 1e9) - last_left_time < reset_mem_time) {
        for (int i = 0; i < mem_size; i++) {
            mem_hand[i] = mem_left_hand[i];
        }
    } else if (inputs.is_left == 0. && float(inputs.timestamp[1] / 1e9) - last_right_time < reset_mem_time) {
        for (int i = 0; i < mem_size; i++) {
            mem_hand[i] = mem_right_hand[i];
        }
    } else {
        AISDK_LOG_TRACE("[GMLPLiftNet3] reset liftnetv3 mem");
        for (int i = 0; i < mem_size; i++) {
            mem_hand[i] = 0;
        }
    }

    for (int i = 0; i < mem_size; i++) {
        AISDK_LOG_TRACE("[GMLPLiftNet3] mem_hand[{}] = {}", i, mem_hand[i]);
    }
}

void GMLPLiftNet3::PostProcess(LiftNetOutputs &outputs, const CamInfo &cam_info, const LiftNetInputs &inputs) {
    int index_output = this->m_net->GetOutputTensorIndex("kpt");

    char *mem = (char *)otensor.m_tensors[index_output].m_viraddr;
    float *_data = (float *)mem;

    outputs.res3d.resize(21);
    float corruption_cam = 0.5;

    for (int i = 0; i < KPT_NUM; i++) {
        auto leftZ = _data;
        auto rightZ = _data + 21;

        auto left_depth = leftZ[i] * baseline_scale_;
        auto right_depth = rightZ[i] * baseline_scale_;

        Eigen::Vector3f leftcam_XYZ;
        Eigen::Vector3f rightcam_XYZ;

        leftcam_XYZ << left_depth * m_leftcam_x[i], left_depth * m_leftcam_y[i], left_depth;
        rightcam_XYZ << right_depth * m_rightcam_x[i], right_depth * m_rightcam_y[i], right_depth;

        leftcam_XYZ.noalias() = rot_left_.transpose() * leftcam_XYZ;
        rightcam_XYZ.noalias() = rot_right_.transpose() * rightcam_XYZ;

        Eigen::Vector3f rightcam_to_left_XYZ = m_cam_info.cvL_T_cvR * rightcam_XYZ;
        Eigen::Vector3f final_kpt = corruption_cam * leftcam_XYZ + (1 - corruption_cam) * rightcam_to_left_XYZ;

        outputs.res3d[i] = cv::Vec3f{final_kpt.x(), final_kpt.y(), final_kpt.z()};
    }

    int index_mem = this->m_net->GetOutputTensorIndex("mem_out");

    int mem_channels = itensor.m_tensors[index_mem].m_dims[0];
    int mem_height = itensor.m_tensors[index_mem].m_dims[1];
    int mem_width = itensor.m_tensors[index_mem].m_dims[2];

    int mem_size = mem_height * mem_width * mem_channels;
    float *mem_hand = (float *)itensor.m_tensors[index_mem].m_viraddr;

    if (inputs.is_left != 0.) {
        for (int i = 0; i < mem_size; i++) {
            mem_left_hand[i] = mem_hand[i];
        }
        last_left_time = float(inputs.timestamp[0] / 1e9);
    } else {
        for (int i = 0; i < mem_size; i++) {
            mem_right_hand[i] = mem_hand[i];
        }
        last_right_time = float(inputs.timestamp[1] / 1e9);
    }
}

}  // namespace aisdk::algorithm
