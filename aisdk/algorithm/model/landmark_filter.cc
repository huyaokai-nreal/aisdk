#include "landmark_filter.h"

#include "../func/NR_CV.h"
#include "../func/permute.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"

namespace aisdk::algorithm {

aisdk::xengine::Status LandmarkFilter::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                            aisdk::xengine::SessionConfig &session) {
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (ret != aisdk::xengine::Status::SUCCESS) {
        return ret;
    }

    itensor = m_net->GetInputTensors();
    otensor = m_net->GetOutputTensors();
    if (aisdk::base::DebugProfiling::Get().GetOpt().aisdk_init_report) {
        PrintfHalIoTensors(itensor);
        PrintfHalIoTensors(otensor);
    }
    itensor_format = checkshapeformat(model.vendor_type, itensor.m_tensors[0].m_rank);
    otensor_format = checkshapeformat(model.vendor_type, otensor.m_tensors[0].m_rank);
    abs_scale.resize(42);
    return aisdk::xengine::Status::SUCCESS;
}

void LandmarkFilter::PreProcess(const std::vector<std::vector<cv::Vec2f>> &net_input) {
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

        std::vector<cv::Vec2f> inputKptS = net_input[0];
        // cv::undistortPoints(inputSeq[0], inputKptS, m_cam_left.k, distcoeff,
        // cv::noArray(), m_cam_left.k);

        m_center_uv = inputKptS[9];
        for (int i = 0; i < inputKptS.size(); i++) {
            inputKptS[i] -= m_center_uv;
            abs_scale[i * 2 + 0] = std::abs(inputKptS[i][0]);
            abs_scale[i * 2 + 1] = std::abs(inputKptS[i][1]);
        }
        m_scale = *std::max_element(abs_scale.begin(), abs_scale.end());

        for (int i = 0; i < net_input.size(); i++) {
            for (int p = 0; p < 21; p++) {
                temp[i * 42 + p * 2] = (net_input[i][p][0] - m_center_uv[0]) / m_scale;
                temp[i * 42 + p * 2 + 1] = (net_input[i][p][1] - m_center_uv[1]) / m_scale;
            }
        }
    }
}

void LandmarkFilter::PostProcess(std::vector<cv::Vec2f> &result) {
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

            result.resize(21);
            for (int i = 0; i < 21; i++) {
                result[i] = {_data[i * 2] * m_scale + m_center_uv[0], _data[i * 2 + 1] * m_scale + m_center_uv[1]};
            }
        }
    }
}

aisdk::xengine::Status LandmarkFilter::Inference(const std::vector<std::vector<cv::Vec2f>> &baseinput,
                                                 std::vector<cv::Vec2f> &baseresult) {
    PreProcess(baseinput);
    aisdk::xengine::Status ret = m_net->RunNet();
    PostProcess(baseresult);
    return ret;
}

}  // namespace aisdk::algorithm
