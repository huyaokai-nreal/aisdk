#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include <numeric>

#include "../func/pose_solver.h"
#include "aisdk/algorithm/common/math.h"
#include "aisdk/algorithm/func/softmax.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "hand_rtmtiny_nimble.h"

namespace aisdk::algorithm {

/**
 * @description:
 * @param {float* restrict} input_hm: input heatmap, 1x32x32x32
 * @param {float* restrict} kpt_x_out: output x coords, normalized to 0-1, 1x21
 * @param {float* restrict} kpt_y_out: output y coords, normalized to 0-1, 1x21
 * @return {*}
 */

absl::Status RTMTinyNimbleDLTFlora::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                         aisdk::xengine::SessionConfig &session) {
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }
    itensor_format_ = itensor.m_tensors[0].m_dimtype;
    otensor_format_ = otensor.m_tensors[0].m_dimtype;
    mul_coeff_ = linspace<float>(0, 1, output_feat_shape_, false);
    mem_left_hand.resize(384);
    mem_right_hand.resize(384);

    AISDK_LOG_TRACE("[RTMTinyNimbleDLTFloraNet] init keypoint_num_ {}", keypoint_num_);

    return ret;
}

void RTMTinyNimbleDLTFlora::PreProcess(const MonoHandNimbleInputs &baseinput) {
    // raw_feats

    int index_feat = this->m_net->GetInputTensorIndex("raw_feats");
    int feat_height = itensor.m_tensors[index_feat].m_dims[0];
    int feat_width = itensor.m_tensors[index_feat].m_dims[1];
    int feat_channels = itensor.m_tensors[index_feat].m_dims[2];
    int feat_size = feat_height * feat_width * feat_channels;

    char *feat_hand_c = (char *)itensor.m_tensors[index_feat].m_viraddr;
    float *feat_hand = (float *)feat_hand_c;
    for (size_t i = 0; i < feat_size; ++i) {
        feat_hand[i] = baseinput.raw_feats[i];
    }
    // scale
    int index_sacle = this->m_net->GetInputTensorIndex("f_scale");
    char *sacle = (char *)itensor.m_tensors[index_sacle].m_viraddr;
    float *sacle_value = (float *)sacle;
    auto l_K = baseinput.virtual_camera->get_camera_intrinsics();
    sacle_value[0] = l_K.fx_ / standard_fscale_;

    // mem_in
    int index_mem = this->m_net->GetInputTensorIndex("in_mems");
    int mem_channels = itensor.m_tensors[index_mem].m_dims[0];
    int mem_height = itensor.m_tensors[index_mem].m_dims[1];
    int mem_width = itensor.m_tensors[index_mem].m_dims[2];
    int element_byte = itensor.m_tensors[index_mem].m_elementbyte;
    int mem_size = mem_height * mem_width * mem_channels;
    char *mem_hand_c = (char *)itensor.m_tensors[index_mem].m_viraddr;
    float *mem_hand = (float *)mem_hand_c;

    if (baseinput.is_left && baseinput.timestamp - last_left_time < reset_mem_time) {
        for (int i = 0; i < mem_size; i++) {
            mem_hand[i] = mem_left_hand[i];
        }
    } else if (!baseinput.is_left && baseinput.timestamp - last_right_time < reset_mem_time) {
        for (int i = 0; i < mem_size; i++) {
            mem_hand[i] = mem_right_hand[i];
            // AISDK_LOG_TRACE("[RTMTinyNimbleDLTFlora] preprocess mem_hand[{}]={}", i, mem_hand[i]);
        }
    } else {
        AISDK_LOG_TRACE("[RTMTinyNimbleDLTFlora] reset liftnimble mem");
        for (int i = 0; i < mem_size; i++) {
            mem_hand[i] = 0;
        }
    }

    // virtual camera
    virtual_camera_ = baseinput.virtual_camera;

    // is_left
    is_left_ = baseinput.is_left;

    // input timestamp
    input_timestamp_ = baseinput.timestamp;
}

void RTMTinyNimbleDLTFlora::PostProcess(MonoHandNimbleOutputs &result) {
    if (!otensor.m_packed_bybatch) {
        return;
    }
    unsigned int _h, _w, _c, element_byte;

    // std::vector<Vec2f_t> rsnkpt;

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
    Eigen::Matrix<float, 5, 3> rel_kpt_else = rel_kpt_26.block(21, 0, 5, 3);

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
    glbal_kpt_v3f.reserve(26);
    for (int i = 0; i < 21; ++i) {
        glbal_kpt_v3f.emplace_back(global_kpt(i, 0), global_kpt(i, 1), global_kpt(i, 2));
    }

    Eigen::Matrix<float, 1, 3> root_3d = global_kpt.block(0, 0, 1, 3);
    for (int i = 0; i < 5; ++i) {
        rel_kpt_else.row(i) += root_3d;
        glbal_kpt_v3f.emplace_back(rel_kpt_else(i, 0), rel_kpt_else(i, 1), rel_kpt_else(i, 2));
    }

    glbal_kpt_v3f = virtual_camera_->eye_to_world(glbal_kpt_v3f);

    AISDK_LOG_TRACE("global_kpt in cam coordinate");
    result.res3d.resize(26);
    for (int i = 0; i < 26; i++) {
        AISDK_LOG_TRACE("{}, {}, {},", glbal_kpt_v3f[i][0], glbal_kpt_v3f[i][1], glbal_kpt_v3f[i][2]);
        result.res3d[i] = Vec3f_t{glbal_kpt_v3f[i][0], glbal_kpt_v3f[i][1], glbal_kpt_v3f[i][2]};
    }
    AISDK_LOG_TRACE("[RTMTinyNimbleDLTFloraNet] infer kpt success");

    // score
    float score = 0.;
    for (float value : sigma) {
        score += 1. / (1. + std::exp(-1. * value));
    }
    score = score / sigma.size();
    result.kpt3d_score = 1 - score;

    // mem_out
    int index_mem = this->m_net->GetOutputTensorIndex("out_mems");

    int mem_channels = otensor.m_tensors[index_mem].m_dims[0];
    int mem_height = otensor.m_tensors[index_mem].m_dims[1];
    int mem_width = otensor.m_tensors[index_mem].m_dims[2];

    int mem_size = mem_height * mem_width * mem_channels;
    float *mem_hand = (float *)otensor.m_tensors[index_mem].m_viraddr;

    if (is_left_ != 0.) {
        for (int i = 0; i < mem_size; i++) {
            mem_left_hand[i] = mem_hand[i];
        }
        last_left_time = input_timestamp_;
    } else {
        for (int i = 0; i < mem_size; i++) {
            mem_right_hand[i] = mem_hand[i];
            // AISDK_LOG_TRACE("[RTMTinyNimbleDLTFlora] postprocess mem_hand[{}]={}", i, mem_right_hand[i]);
        }
        last_right_time = input_timestamp_;
    }
    AISDK_LOG_TRACE("[RTMTinyNimbleDLTFloraNet] infer mem success");
}

absl::StatusOr<MonoHandNimbleOutputs> RTMTinyNimbleDLTFlora::Inference(const MonoHandNimbleInputs &baseinput) {
    {
        // TIMER_ONCE_WITH_TAG(RTMTiny::Preprocess);
        PreProcess(baseinput);
    }

    MonoHandNimbleOutputs baseresult;
    absl::Status ret;

    // 前处理的2d迁移到后处理
    baseresult.kpts.resize(keypoint_num_);
    for (size_t i = 0; i < keypoint_num_; i++) {
        baseresult.kpts[i][0] = baseinput.pred_x[i];
        baseresult.kpts[i][1] = baseinput.pred_y[i];
    }
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
    return absl::UnavailableError("failed to get 3d hand kpt result from RTMTinyNimbleDLTFlora");
}

}  // namespace aisdk::algorithm
