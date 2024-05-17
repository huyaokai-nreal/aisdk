#include "hand_rtmtiny.h"

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include <numeric>

#include "aisdk/algorithm/common/math.h"
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

absl::Status RTMTiny::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                           aisdk::xengine::SessionConfig &session) {
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }
    itensor_format_ = itensor.m_tensors[0].m_dimtype;
    otensor_format_ = otensor.m_tensors[0].m_dimtype;
    input_shape_ = itensor.m_tensors[0].m_dims[1];
    output_shape_ = otensor.m_tensors[0].m_dims[1];
    keypoint_num_ = otensor.m_tensors[0].m_dims[0];
    mul_coeff_ = linspace<float>(0, 1, output_shape_, false);
    return ret;
}

void RTMTiny::PreProcess(const std::vector<Image> &net_input) {
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

void RTMTiny::PostProcess(Kpt2dResult &result) {
    if (!otensor.m_packed_bybatch) {
        return;
    }
    result.kpts.resize(otensor.m_batch);
    result.rdepths.resize(otensor.m_batch);
    unsigned int _h, _w, _c, element_byte;
    for (size_t multi_i = 0; multi_i < otensor.m_multishape_num; multi_i++) {
        for (size_t batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
            _c = 1;
            _h = otensor.m_tensors[multi_i].m_dims[0];
            _w = otensor.m_tensors[multi_i].m_dims[1];
            element_byte = otensor.m_tensors[multi_i].m_elementbyte;
            char *mem = (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * _h * _w * _c * element_byte;
            float *_data = (float *)mem;

            auto &rsnkpt = result.kpts[batch_i];
            auto &rdepth = result.rdepths[batch_i];
            if (rsnkpt.size() != keypoint_num_) {
                rsnkpt.resize(keypoint_num_);
                rdepth.resize(keypoint_num_);
            }
            std::vector<float> kpt_softmax_data(_c * _h * _w);
            softmax_last_dim(_data, kpt_softmax_data.data(), {1, _h, _w});
            for (size_t i = 0; i < keypoint_num_; i++) {
                if (otensor.m_tensors[multi_i].m_name == "feat_x") {
                    rsnkpt[i][0] = std::inner_product(mul_coeff_.begin(), mul_coeff_.end(),
                                                      kpt_softmax_data.begin() + i * _w, 0.0F) *
                                   static_cast<float>(input_shape_);
                }
                if (otensor.m_tensors[multi_i].m_name == "feat_y") {
                    rsnkpt[i][1] = std::inner_product(mul_coeff_.begin(), mul_coeff_.end(),
                                                      kpt_softmax_data.begin() + i * _w, 0.0F) *
                                   static_cast<float>(input_shape_);
                }
                if (otensor.m_tensors[multi_i].m_name == "feat_z") {
                    rdepth[i] = (std::inner_product(mul_coeff_.begin(), mul_coeff_.end(),
                                                    kpt_softmax_data.begin() + i * _w, 0.0F) -
                                 0.5) *
                                0.4;
                }
            }
        }
    }
}

absl::StatusOr<Kpt2dResult> RTMTiny::Inference(const std::vector<Image> &baseinput) {
    PreProcess(baseinput);
    Kpt2dResult baseresult;
    auto ret = m_net->RunNet();
    if (ret.ok()) {
        PostProcess(baseresult);
        return baseresult;
    }
    return absl::UnavailableError("failed to get 2d hand kpt result from rtmtiny");
}

}  // namespace aisdk::algorithm
