#include "hand_detect_artosyn.h"

#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/base/profiling.h"
#include "aisdk/xengine/cv/xr_cv.h"
#include "aisdk/xengine/nrhal_common.h"

namespace aisdk::algorithm {

absl::Status ArtosynHandDetectNetv2::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                          aisdk::xengine::SessionConfig &session) {
    if (model.dont_batch && session.batch > 1) {
        session_batch = session.batch;
        net_batch1 = true;
        session.batch = 1;
    }

    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }

    if (m_input_category != aisdk::xengine::ImageCategory::IS_BLOB) {
        return absl::InternalError("model input is not blob");
    }

    grid_anchor.resize(grid_h * grid_w);
    for (auto i = 0; i < grid_h; i++) {
        for (auto j = 0; j < grid_w; j++) {
            grid_anchor[i * grid_w + j].grid_x = -0.5f + j * 1.0f;
            grid_anchor[i * grid_w + j].grid_y = -0.5f + i * 1.0f;
            grid_anchor[i * grid_w + j].anchor_rw = 31.0f;
            grid_anchor[i * grid_w + j].anchor_rh = 68.0f;
        }
    }

    auto &prof = aisdk::base::DebugProfiling::Get().GetOpt();
    export_netalgo_exec_info = prof.export_pipeline_exec_info_jsonstring;
    AISDK_LOG_TRACE("ArtosynHandDetectNetv2::Inference  export_netalgo_exec_info={}", export_netalgo_exec_info);
    return ret;
}

void ArtosynHandDetectNetv2::PreProcess(const std::vector<Image> &net_input) {
    int ai = iImageblobs.m_batch * iImageblobs.m_multiinput_num;
    int bi = net_input.size();
    if (ai != bi || iImageblobs.m_packed_bybatch == true) {
        return;
    }

    int multi_i, batch_i, height, width, width_s, channels, element_byte;
    for (int i = 0; i < bi; i++) {
        auto &img = net_input[i].m_mat;
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

        char *mem = (char *)iImageblobs.m_imageblobs[index].m_viraddr[0];

        origin_img_width = img.cols;
        origin_img_height = img.rows;

        float wratio = float(width) / float(origin_img_width);
        float hratio = float(height) / float(origin_img_height);

        float ratio = std::min(wratio, hratio);
        int tmp = (ratio < 1.0f) ? cv::INTER_AREA : cv::INTER_LINEAR;
        if (width == width_s) {
            cv::Mat image_resized(cv::Size(width, height), CV_8UC1, mem);
            cv::resize(img, image_resized, cv::Size(width, height), 0, 0, tmp);
        } else {
            cv::Mat dst_resized(cv::Size(width_s, height), CV_8UC1, mem);
            cv::resize(img, dst_resized(cv::Rect(0, 0, width, height)), cv::Size(width, height), 0, 0, tmp);
        }
    }
}

void ArtosynHandDetectNetv2::PostProcess(DetOutputInternal &result) {
    // if (otensor.m_packed_bybatch == false) {
    //     return;
    // }

    AISDK_LOG_TRACE("ArtosynHandDetectNetv2::PostProcess");

    result.images_lhand_rects.resize(otensor.m_batch);
    result.images_rhand_rects.resize(otensor.m_batch);

    int box_c, box_h, box_w, cls_c, cls_h, cls_w, box_element_byte, cls_element_byte;
    int index_box = m_net->GetOutputTensorIndex("output_box");
    int index_cls = m_net->GetOutputTensorIndex("output_cls");
    aisdk::xengine::ArtosynTensorDims &box_dims = otensor.m_tensors[index_box].m_artosyn_dims;
    aisdk::xengine::ArtosynTensorDims &cls_dims = otensor.m_tensors[index_cls].m_artosyn_dims;
    box_c = box_dims.u32OriChannels;
    box_h = box_dims.u32Height;
    box_w = box_dims.u32Width;
    float *box_data = (float *)otensor.m_tensors[index_box].m_viraddr;

    cls_c = cls_dims.u32OriChannels;
    cls_h = cls_dims.u32Height;
    cls_w = cls_dims.u32Width;
    float *cls_data = (float *)otensor.m_tensors[index_cls].m_viraddr;

    for (int batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
        AISDK_LOG_TRACE("ArtosynHandDetectNetv2::Get Results");
        // clang-format off
        AISDK_LOG_TRACE(
            "ArtosynHandDetectNetv2v2:: cls_c: {}, cls_h: {}, cls_w: {}, box_c: {}, box_h: {}, bow_w: {}",
            cls_c, cls_h, cls_w, box_c, box_h, box_w);
        // clang-format on

        std::vector<DetectRect> tmp_result;
        int cls_idx_group, cls_idx_score, cls_idx_left, cls_idx_right;
        int box_idx_group;
        for (int idx_i = 0; idx_i < cls_h; idx_i++) {      // y
            for (int idx_j = 0; idx_j < cls_w; idx_j++) {  // x
                cls_idx_group = ArtosynNpuGetEntryIndex(batch_i, idx_i, idx_j, 0, sizeof(float), cls_dims);
                cls_idx_score = cls_idx_group;
                cls_idx_left = ArtosynNpuGetEntryIndex(batch_i, idx_i, idx_j, 1, sizeof(float), cls_dims);
                cls_idx_right = ArtosynNpuGetEntryIndex(batch_i, idx_i, idx_j, 2, sizeof(float), cls_dims);

                float score = cls_data[cls_idx_score];
                bool is_left = cls_data[cls_idx_left] * score > cls_data[cls_idx_right] * score;
                if (score > score_threshold) {
                    float _coord[box_c];
                    for (int i = 0; i < 4; i++) {
                        box_idx_group = ArtosynNpuGetEntryIndex(batch_i, idx_i, idx_j, i, sizeof(float), box_dims);
                        _coord[i] = box_data[box_idx_group];
                    }

                    uint32_t grid_anchor_index = idx_i * box_w + idx_j;
                    // xywh
                    DetectRect tmp;
                    tmp.w = std::pow(_coord[2] * 2, 2) * grid_anchor[grid_anchor_index].anchor_rw;
                    tmp.h = std::pow(_coord[3] * 2, 2) * grid_anchor[grid_anchor_index].anchor_rh;
                    tmp.x = (_coord[0] * 2 + grid_anchor[grid_anchor_index].grid_x) * grid_stride - tmp.w / 2;
                    tmp.y = (_coord[1] * 2 + grid_anchor[grid_anchor_index].grid_y) * grid_stride - tmp.h / 2;
                    tmp.confidence = score;
                    tmp.left_confidence = cls_data[cls_idx_left];
                    tmp.right_confidence = cls_data[cls_idx_right];
                    tmp.is_left = is_left;
                    tmp.nms_suppressed = false;
                    tmp_result.emplace_back(tmp);
                }
            }
        }

        AISDK_LOG_TRACE("ArtosynHandDetectNetv2v2::tmp_result size(before nms): {}", tmp_result.size());

        auto &lhand_rect = result.images_lhand_rects[batch_i];
        auto &rhand_rect = result.images_rhand_rects[batch_i];

        // nms
        nms(tmp_result, iou_threshold);

        AISDK_LOG_TRACE("ArtosynHandDetectNetv2v2::tmp_result size(after nms): {}", tmp_result.size());

        // scale_coords
        int height = iImageblobs.m_imageblobs[0].m_height;
        int width = iImageblobs.m_imageblobs[0].m_width;

        float min_ratio = std::min(float(width) / float(origin_img_width), float(height) / float(origin_img_height));
        float padx = (float(width) - float(origin_img_width) * min_ratio) / 2;
        float pady = (float(height) - float(origin_img_height) * min_ratio) / 2;
        for (auto &iter : tmp_result) {
            if (iter.nms_suppressed) {
                continue;
            }
            iter.x = std::round((iter.x - padx) / min_ratio);
            iter.y = std::round((iter.y - pady) / min_ratio);
            iter.w = std::round(iter.w / min_ratio);
            iter.h = std::round(iter.h / min_ratio);

            if (true == iter.is_left && lhand_rect.size() == 0) {
                AISDK_LOG_TRACE("push lhand rect: x: {}, y: {}, w: {}, h: {}", iter.x, iter.y, iter.w, iter.h);
                lhand_rect.push_back(iter);
            } else if (false == iter.is_left && rhand_rect.size() == 0) {
                AISDK_LOG_TRACE("push rhand rect: x: {}, y: {}, w: {}, h: {}", iter.x, iter.y, iter.w, iter.h);
                rhand_rect.push_back(iter);
            }
        }
    }
}

void ArtosynHandDetectNetv2::PreProcessSingle(const std::vector<Image> &net_input, uint32_t batchn) {}

void ArtosynHandDetectNetv2::PostProcessSingle(DetOutputInternal &result, uint32_t batchn) {}

absl::Status ArtosynHandDetectNetv2::Inference(const std::vector<Image> &baseinput, DetOutputInternal &baseresult) {
    if (net_batch1) {
        // 后期会删除
        absl::Status ret;
        baseresult.images_lhand_rects.resize(session_batch);
        baseresult.images_rhand_rects.resize(session_batch);

        for (uint32_t i = 0; i < session_batch; i++) {
            PreProcessSingle(baseinput, i);
            ret = m_net->RunNet();
            if (ret.ok()) {
                PostProcessSingle(baseresult, i);
            } else {
                AISDK_LOG_TRACE("ArtosynHandDetectNetv2::Inference  Error!");
            }
        }
        return ret;
    } else {
        PreProcess(baseinput);
        absl::Status ret = m_net->RunNet();
        if (ret.ok()) {
            PostProcess(baseresult);

            AISDK_LOG_TRACE("[ArtosynHandDetectNetv2::Inference] baseresult.images_lhand_rects[0].size(): {}",
                            baseresult.images_lhand_rects[0].size());
            AISDK_LOG_TRACE("[ArtosynHandDetectNetv2::Inference] baseresult.images_lhand_rects[1].size(): {}",
                            baseresult.images_lhand_rects[1].size());
            AISDK_LOG_TRACE("[ArtosynHandDetectNetv2::Inference] baseresult.images_rhand_rects[0].size(): {}",
                            baseresult.images_rhand_rects[0].size());
            AISDK_LOG_TRACE("[ArtosynHandDetectNetv2::Inference] baseresult.images_rhand_rects[1].size(): {}",
                            baseresult.images_rhand_rects[1].size());
        } else {
            baseresult.images_lhand_rects.resize(otensor.m_batch);
            baseresult.images_rhand_rects.resize(otensor.m_batch);
            AISDK_LOG_TRACE("ArtosynHandDetectNetv2::Inference  Error!");
        }
        return ret;
    }
}

}  // namespace aisdk::algorithm
