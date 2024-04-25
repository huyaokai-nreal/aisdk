#include "hand_rsnnano.h"

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/base/log.h"
namespace aisdk::algorithm {
absl::Status RSNNano::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                           aisdk::xengine::SessionConfig &session) {
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }
    itensor_format_ = itensor.m_tensors[0].m_dimtype;
    otensor_format_ = otensor.m_tensors[0].m_dimtype;
    input_shape_ = itensor.m_tensors[0].m_dims[1];
    keypoint_num_ = otensor.m_tensors[0].m_dims[0];
    return ret;
}

void RSNNano::PreProcess(const std::vector<Image> &net_input) {
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

void RSNNano::PostProcess(Kpt2dResult &result) {
    if (!otensor.m_packed_bybatch) {
        return;
    }
    result.kpts.resize(otensor.m_batch);
    unsigned int _h, _w, _c, element_byte;
    for (size_t multi_i = 0; multi_i < otensor.m_multishape_num; multi_i++) {
        for (size_t batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
            _c = otensor.m_tensors[multi_i].m_dims[0];
            _h = otensor.m_tensors[multi_i].m_dims[1];
            _w = 1;
            element_byte = otensor.m_tensors[multi_i].m_elementbyte;
            char *mem = (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * _h * _w * _c * element_byte;
            float *_data = (float *)mem;

            auto &rsnkpt = result.kpts[batch_i];
            if (rsnkpt.size() != keypoint_num_) {
                rsnkpt.resize(keypoint_num_);
            }

            for (size_t i = 0; i < keypoint_num_; i++) {
                rsnkpt[i][0] = _data[i * 3] * static_cast<float>(input_shape_);
                rsnkpt[i][1] = _data[i * 3 + 1] * static_cast<float>(input_shape_);
            }
        }
    }
}

absl::StatusOr<Kpt2dResult> RSNNano::Inference(const std::vector<Image> &baseinput) {
    Kpt2dResult results;
    for (auto &image : baseinput) {
        AISDK_LOG_TRACE("start rsnnano preprocess")
        PreProcess({image});
        AISDK_LOG_TRACE("start rsnnano inference")
        absl::Status ret = m_net->RunNet();
        if (ret.ok()) {
            Kpt2dResult result;
            PostProcess(result);
            AISDK_LOG_TRACE("finish rsnnano inference")
            results.kpts.push_back(std::move(result.kpts[0]));
        } else {
            AISDK_LOG_TRACE("failed rsnnano inference")
            return absl::UnavailableError("failed to get 2d hand kpt result from rsntiny");
        }
    }
    return results;
}

}  // namespace aisdk::algorithm
