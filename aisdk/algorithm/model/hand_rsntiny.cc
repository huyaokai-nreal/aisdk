#include "hand_rsntiny.h"

#include "../func/NR_CV.h"
#include "../func/permute.h"
#include "../func/rsntiny_postprocess.h"
#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"

namespace aisdk::algorithm {

aisdk::xengine::Status RSNTiny::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                     aisdk::xengine::SessionConfig &session) {
    AISDK_LOG_TRACE("Get Here 1?");
    if (model.dont_batch && session.batch > 1) {
        session_batch = session.batch;
        snpe_batch1 = true;
        session.batch = 1;
    }

    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (ret != aisdk::xengine::Status::SUCCESS) {
        AISDK_LOG_TRACE("Get Here 2?");
        return ret;
    }

    AISDK_LOG_TRACE("Get Here 3?");

    // 简单实现
    {
        itensor_format = aisdk::xengine::TensorFormat::CHW;

        AISDK_LOG_TRACE("itensor.m_tensors size: {}", itensor.m_tensors.size());
        AISDK_LOG_TRACE("itensor.m_tensors[0].m_dims: {}", itensor.m_tensors[0].m_dims.size());
        AISDK_LOG_TRACE(" itensor.m_tensors[0].m_rank: {}", itensor.m_tensors[0].m_rank);
        // 这里应该从打包传入dims
        std::vector<uint32_t> iexpect{1, 128, 128};
        if (false == checkshapeformat(itensor.m_tensors[0].m_rank, itensor.m_tensors[0].m_dims, iexpect)) {
            itensor_format = aisdk::xengine::TensorFormat::HWC;
        }
        // SetAlgoParams(std::string("model_input_width"), std::to_string(128));
        // SetAlgoParams(std::string("model_input_height"), std::to_string(128));
    }

    AISDK_LOG_TRACE("Get Here 4?");

    {
        otensor_format = aisdk::xengine::TensorFormat::CHW;
        // 这里应该从打包传入dims
        std::vector<uint32_t> oexpect{21, 32, 32};
        if (false == checkshapeformat(otensor.m_tensors[0].m_rank, otensor.m_tensors[0].m_dims, oexpect)) {
            otensor_format = aisdk::xengine::TensorFormat::HWC;
        }

        uint32_t tmp = 1;
        for (uint32_t i = 0; i < otensor.m_tensors[0].m_rank; i++) {
            tmp *= otensor.m_tensors[0].m_dims[i];
        }
        m_outputsNCHW.resize(tmp);
    }

    AISDK_LOG_TRACE("Get Here 5?");

    return aisdk::xengine::Status::SUCCESS;
}

void RSNTiny::PreProcess(const std::vector<Image> &net_input) {
    int ai = itensor.m_batch * itensor.m_multishape_num;
    int bi = net_input.size();
    if (ai != bi || itensor.m_packed_bybatch == false) {
        return;
    }

    int multi_i, batch_i, height, width, channels, element_byte;
    for (int i = 0; i < bi; i++) {
        auto &img = net_input[i].m_mat;
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

        cv::Mat image_resized;
        img.convertTo(image_resized, CV_32FC1);
        memcpy(mem, image_resized.data, mem_size);
    }
}

void RSNTiny::PostProcess(RSNResult &result) {
    if (otensor.m_packed_bybatch == false) {
        return;
    }

    result.rsn_kpts.resize(otensor.m_batch);
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
            auto &tensorname = otensor.m_tensors[multi_i].m_name;
            element_byte = otensor.m_tensors[multi_i].m_elementbyte;
            char *mem = (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * _h * _w * _c * element_byte;
            float *_data = (float *)mem;

            auto &rsnkpt = result.rsn_kpts[batch_i];
            if (0 == rsnkpt.size()) {
                rsnkpt.resize(KEYPOINT_NUM);
            }

            // 单输出 "feat"
            std::vector<float> kpt_x_data(KEYPOINT_NUM);
            std::vector<float> kpt_y_data(KEYPOINT_NUM);
            if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                rsntiny_postprocess(_data, kpt_x_data.data(), kpt_y_data.data());
            } else if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                NHWC2NCHW(_data, m_outputsNCHW.data(), 1, _c, _h * _w);
                rsntiny_postprocess(m_outputsNCHW.data(), kpt_x_data.data(), kpt_y_data.data());
            }

            for (size_t i = 0; i < KEYPOINT_NUM; i++) {
                rsnkpt[i][0] = kpt_x_data[i] * 128.;
                rsnkpt[i][1] = kpt_y_data[i] * 128.;
            }
        }
    }
}

void RSNTiny::PreProcessSingle(const std::vector<Image> &net_input, uint32_t batchn) {
    int ai = session_batch * itensor.m_multishape_num;
    int bi = net_input.size();
    if (ai != bi || itensor.m_packed_bybatch == false) {
        return;
    }

    int multi_i, batch_i, height, width, channels, element_byte;
    for (int i = 0; i < itensor.m_multishape_num; i++) {
        auto &img = net_input[itensor.m_multishape_num * batchn + i].m_mat;
        multi_i = i;
        batch_i = 0;
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

        cv::Mat image_resized;
        img.convertTo(image_resized, CV_32FC1);
        memcpy(mem, image_resized.data, mem_size);
    }
}

void RSNTiny::PostProcessSingle(RSNResult &result, uint32_t batchn) {
    if (otensor.m_packed_bybatch == false) {
        return;
    }

    int _h, _w, _c, element_byte;
    for (int multi_i = 0; multi_i < otensor.m_multishape_num; multi_i++) {
        for (int batch_i = 0; batch_i < 1; batch_i++) {
            if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                _c = otensor.m_tensors[multi_i].m_dims[0];
                _h = otensor.m_tensors[multi_i].m_dims[1];
                _w = otensor.m_tensors[multi_i].m_dims[2];
            } else if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                _h = otensor.m_tensors[multi_i].m_dims[0];
                _w = otensor.m_tensors[multi_i].m_dims[1];
                _c = otensor.m_tensors[multi_i].m_dims[2];
            }
            auto &tensorname = otensor.m_tensors[multi_i].m_name;
            element_byte = otensor.m_tensors[multi_i].m_elementbyte;
            char *mem = (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * _h * _w * _c * element_byte;
            float *_data = (float *)mem;

            auto &rsnkpt = result.rsn_kpts[batchn];
            if (0 == rsnkpt.size()) {
                rsnkpt.resize(KEYPOINT_NUM);
            }

            // 单输出 "feat"
            std::vector<float> kpt_x_data(KEYPOINT_NUM);
            std::vector<float> kpt_y_data(KEYPOINT_NUM);
            if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                rsntiny_postprocess(_data, kpt_x_data.data(), kpt_y_data.data());
            } else if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                NHWC2NCHW(_data, m_outputsNCHW.data(), 1, _c, _h * _w);
                rsntiny_postprocess(m_outputsNCHW.data(), kpt_x_data.data(), kpt_y_data.data());
            }

            for (size_t i = 0; i < KEYPOINT_NUM; i++) {
                rsnkpt[i][0] = kpt_x_data[i] * 128.;
                rsnkpt[i][1] = kpt_y_data[i] * 128.;
            }
        }
    }
}

aisdk::xengine::Status RSNTiny::Inference(const std::vector<Image> &baseinput, RSNResult &baseresult) {
    if (snpe_batch1) {
        // 后期会删除
        aisdk::xengine::Status ret;
        auto &result = baseresult;
        result.rsn_kpts.resize(session_batch);

        for (uint32_t i = 0; i < session_batch; i++) {
            PreProcessSingle(baseinput, i);
            ret = m_net->RunNet();
            if (ret == aisdk::xengine::Status::SUCCESS) {
                PostProcessSingle(baseresult, i);
            } else {
                AISDK_LOG_TRACE("RSNTiny::Inference  Error!");
            }
        }
        return ret;
    } else {
        PreProcess(baseinput);
        aisdk::xengine::Status ret = m_net->RunNet();
        if (ret == aisdk::xengine::Status::SUCCESS) {
            PostProcess(baseresult);
        } else {
            auto &result = baseresult;
            result.rsn_kpts.resize(otensor.m_batch);
            AISDK_LOG_TRACE("RSNTiny::Inference  Error!");
        }
        return ret;
    }
}

}  // namespace aisdk::algorithm
