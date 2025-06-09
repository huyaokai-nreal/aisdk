#include "hand_rtmtiny_nimble.h"

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include <numeric>

#include "../func/pose_solver.h"
#include "aisdk/algorithm/common/math.h"
#include "aisdk/algorithm/func/softmax.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"

namespace aisdk::algorithm {

/**
 * @description:
 * @param {float* restrict} input_hm: input heatmap, 1x32x32x32
 * @param {float* restrict} kpt_x_out: output x coords, normalized to 0-1, 1x21
 * @param {float* restrict} kpt_y_out: output y coords, normalized to 0-1, 1x21
 * @return {*}
 */

absl::Status RTMTinyNimbleDLT::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                    aisdk::xengine::SessionConfig &session) {
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }
    itensor_format_ = itensor.m_tensors[0].m_dimtype;
    otensor_format_ = otensor.m_tensors[0].m_dimtype;
    mul_coeff_ = linspace<float>(0, 1, output_feat_shape_, false);
    mem_left_hand.resize(105);
    mem_right_hand.resize(105);

    AISDK_LOG_TRACE("[RTMTinyNimbleDLTNet] init keypoint_num_ {}", keypoint_num_);

    return ret;
}

void RTMTinyNimbleDLT::PreProcess(const MonoHandNimbleInputs &baseinput) {
    // auto ai = itensor.m_batch * itensor.m_multishape_num;
    // auto bi = net_input.size();
    // if (ai != bi || !itensor.m_packed_bybatch) {
    //     return;
    // }

    // single image input
    unsigned int height, width, channels, element_byte;
    int index_input = this->m_net->GetInputTensorIndex("input");
    if (itensor_format_ == aisdk::xengine::TensorFormat::CHW) {
        channels = itensor.m_tensors[index_input].m_dims[0];
        height = itensor.m_tensors[index_input].m_dims[1];
        width = itensor.m_tensors[index_input].m_dims[2];
    } else if (itensor_format_ == aisdk::xengine::TensorFormat::HWC) {
        height = itensor.m_tensors[index_input].m_dims[0];
        width = itensor.m_tensors[index_input].m_dims[1];
        channels = itensor.m_tensors[index_input].m_dims[2];
    }
    element_byte = itensor.m_tensors[index_input].m_elementbyte;

    unsigned int mem_size = height * width * channels * element_byte;
    char *mem = (char *)itensor.m_tensors[index_input].m_viraddr;
    auto &img = baseinput.img_input.m_mat;
    cv::Mat image_resized(img.size(), CV_32FC1, mem);
    img.convertTo(image_resized, CV_32FC1);
    image_resized = (image_resized - img_mean_) / img_std_;

    // scale
    int index_sacle = this->m_net->GetInputTensorIndex("f_scale");
    char *sacle = (char *)itensor.m_tensors[index_sacle].m_viraddr;
    float *sacle_value = (float *)sacle;
    auto l_K = baseinput.virtual_camera->get_camera_intrinsics();
    sacle_value[0] = l_K.fx_ / standard_fscale_;

    // mem_in
    // int index_mem = this->m_net->GetInputTensorIndex("mem_in");
    // int mem_channels = itensor.m_tensors[index_mem].m_dims[0];
    // int mem_height = itensor.m_tensors[index_mem].m_dims[1];
    // int mem_width = itensor.m_tensors[index_mem].m_dims[2];
    // element_byte = itensor.m_tensors[index_mem].m_elementbyte;
    // mem_size = mem_height * mem_width * mem_channels;
    // char *mem_hand_c = (char *)itensor.m_tensors[index_mem].m_viraddr;
    // float *mem_hand = (float *)mem_hand_c;

    // if (baseinput.is_left && baseinput.timestamp - last_left_time < reset_mem_time) {
    //     for (int i = 0; i < mem_size; i++) {
    //         mem_hand[i] = mem_left_hand[i];
    //     }
    // } else if (!baseinput.is_left && baseinput.timestamp - last_right_time < reset_mem_time) {
    //     for (int i = 0; i < mem_size; i++) {
    //         mem_hand[i] = mem_right_hand[i];
    //     }
    // } else {
    //     AISDK_LOG_TRACE("[RTMTinyNimbleDLT] reset liftnimble mem");
    //     for (int i = 0; i < mem_size; i++) {
    //         mem_hand[i] = 0;
    //     }
    // }

    // virtual camera
    virtual_camera_ = baseinput.virtual_camera;

    // is_left
    is_left_ = baseinput.is_left;

    // input timestamp
    input_timestamp_ = baseinput.timestamp;
}

void RTMTinyNimbleDLT::PostProcess(MonoHandNimbleOutputs &result) {
    if (!otensor.m_packed_bybatch) {
        return;
    }
    unsigned int _h, _w, _c, element_byte;

    // std::vector<Vec2f_t> rsnkpt;
    result.kpts.resize(keypoint_num_);

    // feat_x
    int index_feat_x = this->m_net->GetOutputTensorIndex("feat_x");
    _c = 1;
    _h = otensor.m_tensors[index_feat_x].m_dims[0];
    _w = otensor.m_tensors[index_feat_x].m_dims[1];
    element_byte = otensor.m_tensors[index_feat_x].m_elementbyte;
    char *feat_x_mem = (char *)otensor.m_tensors[index_feat_x].m_viraddr;
    float *feat_x_data = (float *)feat_x_mem;
    std::vector<float> feat_x_softmax_data(_c * _h * _w);
    softmax_last_dim(feat_x_data, feat_x_softmax_data.data(), {1, _h, _w});
    for (size_t i = 0; i < keypoint_num_; i++) {
        result.kpts[i][0] =
            std::inner_product(mul_coeff_.begin(), mul_coeff_.end(), feat_x_softmax_data.begin() + i * _w, 0.0F) *
            static_cast<float>(input_image_shape_);
    }

    // feat_y
    int index_feat_y = this->m_net->GetOutputTensorIndex("feat_y");
    _c = 1;
    _h = otensor.m_tensors[index_feat_y].m_dims[0];
    _w = otensor.m_tensors[index_feat_y].m_dims[1];
    element_byte = otensor.m_tensors[index_feat_y].m_elementbyte;
    char *feat_y_mem = (char *)otensor.m_tensors[index_feat_y].m_viraddr;
    float *feat_y_data = (float *)feat_y_mem;
    std::vector<float> feat_y_softmax_data(_c * _h * _w);
    softmax_last_dim(feat_y_data, feat_y_softmax_data.data(), {1, _h, _w});

    for (size_t i = 0; i < keypoint_num_; i++) {
        result.kpts[i][1] =
            std::inner_product(mul_coeff_.begin(), mul_coeff_.end(), feat_y_softmax_data.begin() + i * _w, 0.0F) *
            static_cast<float>(input_image_shape_);
    }

    // svd
    int index_svd_pt = this->m_net->GetOutputTensorIndex("svd_pt");
    float *svd_pt_ptr = (float *)otensor.m_tensors[index_svd_pt].m_viraddr;
    Eigen::Map<Eigen::Matrix<float, 7, 3, Eigen::RowMajor>> svd_pt(svd_pt_ptr);
    Eigen::Matrix<float, 7, 3> svd_src_pt;
    svd_src_pt << 0., 0., 0., 0.1, 0., 0., 0., 0.1, 0., 0., 0., 0.1, -0.07071068, -0.07071068, 0., -0.07071068, 0.,
        -0.07071068, 0., -0.07071068, -0.07071068;
    Eigen::Isometry3f global_hand_pose = get_transform_with_svd(svd_src_pt, svd_pt);
    Eigen::Matrix3f global_rotation = global_hand_pose.rotation();

    AISDK_LOG_TRACE("is_left {} ", is_left_);
    if (is_left_) {
        // flip kpt2d
        for (size_t i = 0; i < keypoint_num_; i++) {
            result.kpts[i][0] = input_image_shape_ - result.kpts[i][0];
        }

        // flip nimble coefficients
        Eigen::Matrix3f flip_x;
        flip_x << -1, 0, 0, 0, 1, 0, 0, 0, 1;
        global_rotation.noalias() = flip_x * global_rotation;
    }

    // shape

    float shape_param = 0.0;
    // GlobalPredictorService::getInstance().update_hand_scale(shape_param + 1);
    // shape_param = GlobalPredictorService::getInstance().get_hand_scale() - 1;
    int index_angle = this->m_net->GetOutputTensorIndex("angle");
    float *angle_ptr = (float *)otensor.m_tensors[index_angle].m_viraddr;
    std::vector<float> local_angles(angle_ptr, angle_ptr + 171);

    local_angles = decode_hand_angle(local_angles);
    auto local_kpt = decode_hand_joints(shape_param, local_angles);

    Eigen::Matrix<float, 26, 3> rel_kpt_26 = (global_rotation * local_kpt.transpose()).transpose();

    Eigen::Matrix<float, 21, 3> rel_kpt = rel_kpt_26.block(0, 0, 21, 3);

    Eigen::Matrix3f intrix_matrix;
    auto l_K = virtual_camera_->get_camera_intrinsics();
    intrix_matrix << l_K.fx_, 0, l_K.cx_, 0, l_K.fy_, l_K.cy_, 0, 0, 1;

    // sigma
    int index_sigma = this->m_net->GetOutputTensorIndex("sigma");
    float *sigma_ptr = (float *)otensor.m_tensors[index_sigma].m_viraddr;
    std::vector<float> sigma(sigma_ptr, sigma_ptr + 63);

    Eigen::MatrixXf kpt_weight = cal_kpt_weight(sigma);

    Eigen::Matrix<float, 21, 3> global_kpt = get_3d_kpt(rel_kpt, result.kpts, intrix_matrix, kpt_weight);

    std::vector<Eigen::Vector3f> glbal_kpt_v3f;
    glbal_kpt_v3f.reserve(21);
    for (int i = 0; i < 21; ++i) {
        glbal_kpt_v3f.emplace_back(global_kpt(i, 0), global_kpt(i, 1), global_kpt(i, 2));
    }

    glbal_kpt_v3f = virtual_camera_->eye_to_world(glbal_kpt_v3f);

    AISDK_LOG_TRACE("global_kpt in cam coordinate");
    result.res3d.resize(21);
    for (int i = 0; i < kAlgoKeypointNum; i++) {
        AISDK_LOG_TRACE("{}, {}, {},", glbal_kpt_v3f[i][0], glbal_kpt_v3f[i][1], glbal_kpt_v3f[i][2]);
        result.res3d[i] = Vec3f_t{glbal_kpt_v3f[i][0], glbal_kpt_v3f[i][1], glbal_kpt_v3f[i][2]};
    }
    AISDK_LOG_TRACE("[RTMTinyNimbleDLTNet] infer kpt success");

    // score
    int index_score = this->m_net->GetOutputTensorIndex("score");
    float score = *((float *)otensor.m_tensors[index_score].m_viraddr);
    result.kpt3d_score = 1 - score;

    // mem_out
    // int index_mem = this->m_net->GetOutputTensorIndex("mem_out");

    // int mem_channels = otensor.m_tensors[index_mem].m_dims[0];
    // int mem_height = otensor.m_tensors[index_mem].m_dims[1];
    // int mem_width = otensor.m_tensors[index_mem].m_dims[2];

    // int mem_size = mem_height * mem_width * mem_channels;
    // float *mem_hand = (float *)otensor.m_tensors[index_mem].m_viraddr;

    // if (is_left_!= 0.) {
    //     for (int i = 0; i < mem_size; i++) {
    //         mem_left_hand[i] = mem_hand[i];
    //     }
    //     last_left_time = input_timestamp_;
    // } else {
    //     for (int i = 0; i < mem_size; i++) {
    //         mem_right_hand[i] = mem_hand[i];
    //     }
    //     last_right_time = input_timestamp_;
    // }
    AISDK_LOG_TRACE("[RTMTinyNimbleDLTNet] infer mem success");
}

absl::StatusOr<MonoHandNimbleOutputs> RTMTinyNimbleDLT::Inference(const MonoHandNimbleInputs &baseinput) {
    {
        // TIMER_ONCE_WITH_TAG(RTMTiny::Preprocess);
        PreProcess(baseinput);
    }

    MonoHandNimbleOutputs baseresult;
    absl::Status ret;
    {
        // TIMER_ONCE_WITH_TAG(RTMTiny::RunNet);
        ret = m_net->RunNet();
    }
    if (ret.ok()) {
        {
            // TIMER_ONCE_WITH_TAG(RTMTiny::PoseProcess);
            PostProcess(baseresult);
        }
        return baseresult;
    }
    return absl::UnavailableError("failed to get 3d hand kpt result from RTMTinyNimbleDLT");
}

}  // namespace aisdk::algorithm
