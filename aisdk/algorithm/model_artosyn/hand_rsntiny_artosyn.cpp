#include "hand_rsntiny_artosyn.h"

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include <cstdint>

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

void ArtosynRSNTiny::ipr(float *__restrict input_hm, float *__restrict kpt_x_out, float *__restrict kpt_y_out) {
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

absl::Status ArtosynRSNTiny::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                  aisdk::xengine::SessionConfig &session) {
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }

    if (m_input_category != aisdk::xengine::ImageCategory::IS_BLOB) {
        return absl::InternalError("model input is not blob");
    }

    int index_input = m_net->GetInputImageBlobsIndex("input");
    input_shape_ = iImageblobs.m_imageblobs[index_input].m_width;
    int feat_c, feat_h, feat_w, feat_element_byte;
    int index_feat = m_net->GetOutputTensorIndex("feat");
    aisdk::xengine::ArtosynTensorDims &feat_dims = otensor.m_tensors[index_feat].m_artosyn_dims;
    feat_c = feat_dims.u32OriChannels;
    feat_h = feat_dims.u32Height;
    feat_w = feat_dims.u32Width;
    m_outputsNCHW.resize(feat_c * feat_h * feat_w);
    // // resize post process memory
    output_shape_ = feat_w;
    keypoint_num_ = feat_c;
    hm_softmax_.resize(keypoint_num_ * output_shape_ * output_shape_);
    hm_reduce_col_.resize(keypoint_num_ * output_shape_);
    hm_reduce_row_.resize(keypoint_num_ * output_shape_);
    hm_reduce_col_row_.resize(keypoint_num_ * output_shape_);
    hm_reduce_row_col_.resize(keypoint_num_ * output_shape_);
    // init ipr coeff map
    mul_coeff_ = linspace<float>(0, 1, output_shape_, false);
    return ret;
}

void ArtosynRSNTiny::PreProcess(const std::vector<Image> &net_input) {
    int ai = iImageblobs.m_batch * iImageblobs.m_multiinput_num;
    int bi = net_input.size();
    if (ai != bi || iImageblobs.m_packed_bybatch == true) {
        return;
    }

    int multi_i, batch_i, height, width, width_s, channels, element_byte;
    for (int i = 0; i < bi; i++) {
        auto &img = net_input[i].m_mat;

        int width = 128;
        int height = 128;

        multi_i = i / iImageblobs.m_batch;
        batch_i = i % iImageblobs.m_batch;
        auto index = i;
        if (iImageblobs.m_imageblobs[index].m_format == aisdk::xengine::ImageFormat::GRAY) {
            channels = 1;
        } else {
            continue;
        }
        height = iImageblobs.m_imageblobs[index].m_height;
        width = iImageblobs.m_imageblobs[index].m_width;
        width_s = iImageblobs.m_imageblobs[index].m_wstride;
        element_byte = iImageblobs.m_imageblobs[index].m_elementbyte;

        char *src_mem = (char *)img.data;
        char *dst_mem = (char *)iImageblobs.m_imageblobs[index].m_viraddr[0];

        if (width == width_s) {
            unsigned int mem_size = height * width * channels * element_byte;
            memcpy(dst_mem, src_mem, mem_size);
        } else {
            for (uint32_t hi = 0; hi < height; hi++) {
                memcpy(dst_mem, src_mem, width);
                memset(dst_mem + width, 0, width_s - width);
                src_mem += width;
                dst_mem += width_s;
            }
        }
    }
}

void ArtosynRSNTiny::PostProcess(Kpt2dResult &result) {
    int depth_c, depth_h, depth_w, feat_c, feat_h, feat_w, depth_element_byte, feat_element_byte;
    int index_depth = m_net->GetOutputTensorIndex("depth");
    int index_feat = m_net->GetOutputTensorIndex("feat");
    aisdk::xengine::ArtosynTensorDims &depth_dims = otensor.m_tensors[index_depth].m_artosyn_dims;
    aisdk::xengine::ArtosynTensorDims &feat_dims = otensor.m_tensors[index_feat].m_artosyn_dims;
    depth_c = depth_dims.u32OriChannels;
    depth_h = depth_dims.u32Height;
    depth_w = depth_dims.u32Width;
    float *depth_data = (float *)otensor.m_tensors[index_depth].m_viraddr;

    feat_c = feat_dims.u32OriChannels;
    feat_h = feat_dims.u32Height;
    feat_w = feat_dims.u32Width;
    float *feat_data = (float *)otensor.m_tensors[index_feat].m_viraddr;
    result.kpts.resize(otensor.m_batch);
    for (int batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
        auto &rsnkpt = result.kpts[batch_i];
        rsnkpt.resize(keypoint_num_);

        int feat_idx_group, feat_idx_score, feat_idx_left, feat_idx_right;
        std::vector<float> storedata(feat_c * feat_h * feat_w);
        for (int idx_c = 0; idx_c < feat_c; idx_c++) {
            for (int idx_i = 0; idx_i < feat_h; idx_i++) {      // y
                for (int idx_j = 0; idx_j < feat_w; idx_j++) {  // x
                    feat_idx_group = ArtosynNpuGetEntryIndex(batch_i, otensor.m_batch, idx_i, idx_j, idx_c,
                                                             sizeof(float), feat_dims);
                    storedata[idx_c * feat_h * feat_w + idx_i * feat_w + idx_j] = feat_data[feat_idx_group];
                }
            }
        }

        // 单输出 "feat"
        std::vector<float> kpt_x_data(keypoint_num_);
        std::vector<float> kpt_y_data(keypoint_num_);
        ipr(storedata.data(), kpt_x_data.data(), kpt_y_data.data());

        for (size_t i = 0; i < keypoint_num_; i++) {
            rsnkpt[i][0] = kpt_x_data[i] * static_cast<float>(input_shape_);
            rsnkpt[i][1] = kpt_y_data[i] * static_cast<float>(input_shape_);
        }
    }
}

absl::StatusOr<Kpt2dResult> ArtosynRSNTiny::Inference(const std::vector<Image> &baseinput) {
    PreProcess(baseinput);
    Kpt2dResult baseresult;
    absl::Status ret = m_net->RunNet();
    if (ret.ok()) {
        PostProcess(baseresult);
        return baseresult;
    }
    return absl::UnavailableError("failed to get 2d hand kpt result from ArtosynRSNTiny");
}

}  // namespace aisdk::algorithm
