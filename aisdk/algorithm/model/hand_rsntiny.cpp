#include "hand_rsntiny.h"

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include "aisdk/algorithm/common/math.h"
#include "aisdk/algorithm/func/elementwise_mul.h"
#include "aisdk/algorithm/func/permute.h"
#include "aisdk/algorithm/func/reducesum.h"
#include "aisdk/algorithm/func/softmax.h"
#include "aisdk/base/log.h"
namespace aisdk::algorithm {

/**
 * @description:
 * @param {float* restrict} input_hm: input heatmap, 1x32x32x32
 * @param {float* restrict} kpt_x_out: output x coords, normalized to 0-1, 1x21
 * @param {float* restrict} kpt_y_out: output y coords, normalized to 0-1, 1x21
 * @return {*}
 */

void RSNTiny::ipr(float *__restrict input_hm, float *__restrict kpt_x_out, float *__restrict kpt_y_out) {
    softmax_last_dim(input_hm, hm_softmax_.data(), {1, keypoint_num_, output_shape_ * output_shape_});

    reduce_sum_h(hm_softmax_.data(), hm_reduce_col_.data(), keypoint_num_, output_shape_, output_shape_);
    reduce_sum_w(hm_softmax_.data(), hm_reduce_row_.data(), keypoint_num_, output_shape_, output_shape_);

    elementwise_mult_hm(hm_reduce_col_.data(), mul_coeff_.data(), hm_reduce_col_row_.data(), keypoint_num_,
                        keypoint_num_ * output_shape_);
    elementwise_mult_hm(hm_reduce_row_.data(), mul_coeff_.data(), hm_reduce_row_col_.data(), keypoint_num_,
                        keypoint_num_ * output_shape_);

    reduce_sum_w(hm_reduce_col_row_.data(), kpt_x_out, keypoint_num_, 1, output_shape_);
    reduce_sum_h(hm_reduce_row_col_.data(), kpt_y_out, keypoint_num_, output_shape_, 1);
}

aisdk::xengine::Status RSNTiny::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                     aisdk::xengine::SessionConfig &session) {
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (ret != aisdk::xengine::Status::SUCCESS) {
        return ret;
    }
    itensor_format_ = itensor.m_tensors[0].m_dimtype;
    otensor_format_ = otensor.m_tensors[0].m_dimtype;
    m_outputsNCHW.resize(otensor.m_tensors[0].m_elementsize);
    // resize post process memory
    input_shape_ = itensor.m_tensors[0].m_dims[1];
    output_shape_ = otensor.m_tensors[0].m_dims[1];
    keypoint_num_ = otensor.m_tensors[0].m_dims[0];
    hm_softmax_.resize(keypoint_num_ * output_shape_ * output_shape_);
    hm_reduce_col_.resize(keypoint_num_ * output_shape_);
    hm_reduce_row_.resize(keypoint_num_ * output_shape_);
    hm_reduce_col_row_.resize(keypoint_num_ * output_shape_);
    hm_reduce_row_col_.resize(keypoint_num_ * output_shape_);
    // init ipr coeff map
    mul_coeff_ = linspace<float>(0, 1, output_shape_, false);
    return aisdk::xengine::Status::SUCCESS;
}

void RSNTiny::PreProcess(const std::vector<Image> &net_input) {
    auto ai = itensor.m_batch * itensor.m_multishape_num;
    auto bi = net_input.size();
    if (ai != bi || !itensor.m_packed_bybatch) {
        return;
    }

    unsigned int multi_i, batch_i, height, width, channels, element_byte;
    for (size_t i = 0; i < bi; i++) {
        auto &img = net_input[i].m_mat;
        multi_i = i / itensor.m_batch;
        batch_i = i % itensor.m_batch;
        if (itensor_format_ == aisdk::xengine::TensorFormat::CHW) {
            channels = itensor.m_tensors[multi_i].m_dims[0];
            height = itensor.m_tensors[multi_i].m_dims[1];
            width = itensor.m_tensors[multi_i].m_dims[2];
        } else if (itensor_format_ == aisdk::xengine::TensorFormat::HWC) {
            height = itensor.m_tensors[multi_i].m_dims[0];
            width = itensor.m_tensors[multi_i].m_dims[1];
            channels = itensor.m_tensors[multi_i].m_dims[2];
        }
        element_byte = itensor.m_tensors[multi_i].m_elementbyte;

        unsigned int mem_size = height * width * channels * element_byte;
        char *mem = (char *)itensor.m_tensors[multi_i].m_viraddr + batch_i * mem_size;

        cv::Mat image_resized;
        img.convertTo(image_resized, CV_32FC1);
        memcpy(mem, image_resized.data, mem_size);
    }
}

void RSNTiny::PostProcess(Kpt2dResult &result) {
    if (!otensor.m_packed_bybatch) {
        return;
    }
    result.rsn_kpts.resize(otensor.m_batch);
    unsigned int _h, _w, _c, element_byte;
    for (size_t multi_i = 0; multi_i < otensor.m_multishape_num; multi_i++) {
        for (size_t batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
            if (otensor_format_ == aisdk::xengine::TensorFormat::CHW) {
                _c = otensor.m_tensors[multi_i].m_dims[0];
                _h = otensor.m_tensors[multi_i].m_dims[1];
                _w = otensor.m_tensors[multi_i].m_dims[2];
            } else if (otensor_format_ == aisdk::xengine::TensorFormat::HWC) {
                _h = otensor.m_tensors[multi_i].m_dims[0];
                _w = otensor.m_tensors[multi_i].m_dims[1];
                _c = otensor.m_tensors[multi_i].m_dims[2];
            }
            element_byte = otensor.m_tensors[multi_i].m_elementbyte;
            char *mem = (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * _h * _w * _c * element_byte;
            float *_data = (float *)mem;

            auto &rsnkpt = result.rsn_kpts[batch_i];
            if (0 == rsnkpt.size()) {
                rsnkpt.resize(keypoint_num_);
            }

            // 单输出 "feat"
            std::vector<float> kpt_x_data(keypoint_num_);
            std::vector<float> kpt_y_data(keypoint_num_);
            if (otensor_format_ == aisdk::xengine::TensorFormat::CHW) {
                ipr(_data, kpt_x_data.data(), kpt_y_data.data());
            } else if (otensor_format_ == aisdk::xengine::TensorFormat::HWC) {
                NHWC2NCHW(_data, m_outputsNCHW.data(), 1, _c, _h * _w);
                ipr(m_outputsNCHW.data(), kpt_x_data.data(), kpt_y_data.data());
            }

            for (size_t i = 0; i < keypoint_num_; i++) {
                rsnkpt[i][0] = kpt_x_data[i] * static_cast<float>(input_shape_);
                rsnkpt[i][1] = kpt_y_data[i] * static_cast<float>(input_shape_);
            }
        }
    }
}

absl::StatusOr<Kpt2dResult> RSNTiny::Inference(const std::vector<Image> &baseinput) {
    PreProcess(baseinput);
    Kpt2dResult baseresult;
    aisdk::xengine::Status ret = m_net->RunNet();
    if (ret == aisdk::xengine::Status::SUCCESS) {
        PostProcess(baseresult);
        return baseresult;
    }
    return absl::UnavailableError("failed to get 2d hand kpt result from rsntiny");
}

}  // namespace aisdk::algorithm
