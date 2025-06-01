#include "hand_detect.h"

#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/base/profiling.h"
#include "aisdk/xengine/cv/xr_cv.h"

namespace aisdk::algorithm {

absl::Status HandDetectNet::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
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

    // 简单实现
    {
        itensor_format = checkshapeformat(model.vendor_type, itensor.m_tensors[0].m_rank);
        int height = 0;
        int width = 0;
        if (itensor_format == aisdk::xengine::TensorFormat::CHW) {
            height = itensor.m_tensors[0].m_dims[1];
            width = itensor.m_tensors[0].m_dims[2];
        } else if (itensor_format == aisdk::xengine::TensorFormat::HWC) {
            height = itensor.m_tensors[0].m_dims[0];
            width = itensor.m_tensors[0].m_dims[1];
        }

        grid_h = height / grid_stride;
        grid_w = width / grid_stride;
    }

    { otensor_format = checkshapeformat(model.vendor_type, otensor.m_tensors[0].m_rank); }

    grid_anchor.resize(grid_h * grid_w);
    for (auto i = 0; i < grid_h; i++) {
        for (auto j = 0; j < grid_w; j++) {
            grid_anchor[i * grid_w + j].grid_x = -0.5f + j * 1.0f;
            grid_anchor[i * grid_w + j].grid_y = -0.5f + i * 1.0f;
            grid_anchor[i * grid_w + j].anchor_rw = 33.0f;
            grid_anchor[i * grid_w + j].anchor_rh = 30.0f;
        }
    }

    auto &prof = aisdk::base::DebugProfiling::Get().GetOpt();
    export_netalgo_exec_info = prof.export_pipeline_exec_info_jsonstring;
    AISDK_LOG_TRACE("HandDetectNet::Inference  export_netalgo_exec_info={}", export_netalgo_exec_info);
    return ret;
}

void HandDetectNet::PreProcess(const std::vector<Image> &net_input) {
    int ai = itensor.m_batch * itensor.m_multishape_num;
    int bi = net_input.size();
    if (ai != bi || itensor.m_packed_bybatch == false) {
        AISDK_LOG_ERROR(
            "ai[{}] not equal to bi[{}] or m_apcked_bybatch[{}] is false, do not do preprocess. itensor.m_batch[{}], "
            "itensor.m_multishape_num[{}]",
            ai, bi, itensor.m_packed_bybatch, itensor.m_batch, itensor.m_multishape_num);
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
        float _mean = 0.0f;
        float _norm = 255.0f;

        origin_img_width = img.cols;
        origin_img_height = img.rows;

        float wratio = float(width) / float(origin_img_width);
        float hratio = float(height) / float(origin_img_height);
#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
        if ((wratio - 0.4f) < 1e-5 && (hratio - 0.4f) < 1e-5) {
            cv::Mat image_resized(cv::Size(width, height), CV_32FC1, mem);
            // 仅支持等比例缩小2.5倍
            aisdk::xengine::NrResize((unsigned char *)img.data, (float *)image_resized.data, height, width, 1.f, _mean,
                                     _norm);
        } else
#endif
        {
            float ratio = std::min(wratio, hratio);
            int tmp = (ratio < 1.0f) ? cv::INTER_AREA : cv::INTER_LINEAR;
            cv::Mat image_resized(cv::Size(width, height), CV_8UC1);
            cv::resize(img, image_resized, cv::Size(width, height), 0, 0, tmp);
            image_resized.convertTo(image_resized, CV_32FC1);
            cv::Mat new_mat(cv::Size(width, height), CV_32FC1, mem);
            new_mat = (image_resized - _mean) / _norm;
        }
    }
}

void HandDetectNet::PostProcess(DetOutputInternal &result) {
    if (otensor.m_packed_bybatch == false) {
        return;
    }

    result.images_lhand_rects.resize(otensor.m_batch);
    result.images_rhand_rects.resize(otensor.m_batch);

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

            if (_c != FEATURE_NUM || _h != grid_h || _w != grid_w) {
                continue;
            }

            std::vector<DetectRect> tmp_result;
            int _idx_group, _idx_score, _idx_left, _idx_right;
            for (int idx_i = 0; idx_i < _h; idx_i++) {      // y
                for (int idx_j = 0; idx_j < _w; idx_j++) {  // x
                    if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                        _idx_group = idx_i * _w * _c + idx_j * _c;
                        _idx_score = _idx_group + 4;
                        _idx_left = _idx_group + 5;
                        _idx_right = _idx_group + 6;
                    } else if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                        _idx_score = 4 * _h * _w + idx_i * _w + idx_j;
                        _idx_left = 5 * _h * _w + idx_i * _w + idx_j;
                        _idx_right = 6 * _h * _w + idx_i * _w + idx_j;
                    }
                    float score = _data[_idx_score];
                    bool is_left = _data[_idx_left] * score > _data[_idx_right] * score;
                    if (score > score_threshold) {
                        float _coord[_c];
                        if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                            for (int i = 0; i < 4; i++) {
                                _coord[i] = _data[_idx_group + i];
                            }
                        } else if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                            for (int i = 0; i < 4; i++) {
                                int _val_idx = i * _h * _w + idx_i * _w + idx_j;
                                _coord[i] = _data[_val_idx];
                            }
                        }

                        uint32_t grid_anchor_index = idx_i * _w + idx_j;
                        // xywh
                        DetectRect tmp;
                        tmp.w = std::pow(_coord[2] * 2, 2) * grid_anchor[grid_anchor_index].anchor_rw;
                        tmp.h = std::pow(_coord[3] * 2, 2) * grid_anchor[grid_anchor_index].anchor_rh;
                        tmp.x = (_coord[0] * 2 + grid_anchor[grid_anchor_index].grid_x) * grid_stride - tmp.w / 2;
                        tmp.y = (_coord[1] * 2 + grid_anchor[grid_anchor_index].grid_y) * grid_stride - tmp.h / 2;
                        tmp.confidence = score;
                        tmp.left_confidence = _data[_idx_left];
                        tmp.right_confidence = _data[_idx_right];
                        tmp.is_left = is_left;
                        tmp.nms_suppressed = false;
                        tmp_result.emplace_back(tmp);
                    }
                }
            }

            AISDK_LOG_TRACE("tmp_result.size(): {}", tmp_result.size());

            auto &lhand_rect = result.images_lhand_rects[batch_i];
            auto &rhand_rect = result.images_rhand_rects[batch_i];

            // nms
            nms(tmp_result, iou_threshold);

            // scale_coords
            int height, width;
            if (itensor_format == aisdk::xengine::TensorFormat::CHW) {
                height = itensor.m_tensors[multi_i].m_dims[1];
                width = itensor.m_tensors[multi_i].m_dims[2];
            } else if (itensor_format == aisdk::xengine::TensorFormat::HWC) {
                height = itensor.m_tensors[multi_i].m_dims[0];
                width = itensor.m_tensors[multi_i].m_dims[1];
            }

            float min_ratio =
                std::min(float(width) / float(origin_img_width), float(height) / float(origin_img_height));
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
                    AISDK_LOG_TRACE("push lhand rect: {}", iter.x);
                    lhand_rect.push_back(iter);
                } else if (false == iter.is_left && rhand_rect.size() == 0) {
                    AISDK_LOG_TRACE("push rhand rect: {}", iter.x);
                    rhand_rect.push_back(iter);
                }
            }
        }
    }
}

void HandDetectNet::PreProcessSingle(const std::vector<Image> &net_input, uint32_t batchn) {
    int ai = session_batch * itensor.m_multishape_num;
    int bi = net_input.size();
    if (ai != bi || itensor.m_packed_bybatch == false) {
        AISDK_LOG_ERROR(
            "ai[{}] not equal to bi[{}] or m_apcked_bybatch[{}] is false, do not do preprocess. session_batch[{}], "
            "itensor.m_multishape_num[{}]",
            ai, bi, itensor.m_packed_bybatch, session_batch, itensor.m_multishape_num);
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
        float _mean = 0.0f;
        float _norm = 255.0f;

        origin_img_width = img.cols;
        origin_img_height = img.rows;

        float wratio = float(width) / float(origin_img_width);
        float hratio = float(height) / float(origin_img_height);
#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
        if ((wratio - 0.4f) < 1e-5 && (hratio - 0.4f) < 1e-5) {
            cv::Mat image_resized(cv::Size(width, height), CV_32FC1, mem);
            // 仅支持等比例缩小2.5倍
            aisdk::xengine::NrResize((unsigned char *)img.data, (float *)image_resized.data, height, width, 1.f, _mean,
                                     _norm);
        } else
#endif
        {
            float ratio = std::min(wratio, hratio);
            int tmp = (ratio < 1.0f) ? cv::INTER_AREA : cv::INTER_LINEAR;
            cv::Mat image_resized(cv::Size(width, height), CV_8UC1);
            cv::resize(img, image_resized, cv::Size(width, height), 0, 0, tmp);
            image_resized.convertTo(image_resized, CV_32FC1);
            cv::Mat new_mat(cv::Size(width, height), CV_32FC1, mem);
            new_mat = (image_resized - _mean) / _norm;
        }
    }
}

void HandDetectNet::PostProcessSingle(DetOutputInternal &result, uint32_t batchn) {
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
            element_byte = otensor.m_tensors[multi_i].m_elementbyte;
            char *mem = (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * _h * _w * _c * element_byte;
            float *_data = (float *)mem;

            if (_c != FEATURE_NUM || _h != grid_h || _w != grid_w) {
                continue;
            }

            std::vector<DetectRect> tmp_result;
            int _idx_group, _idx_score, _idx_left, _idx_right;
            for (int idx_i = 0; idx_i < _h; idx_i++) {      // y
                for (int idx_j = 0; idx_j < _w; idx_j++) {  // x
                    if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                        _idx_group = idx_i * _w * _c + idx_j * _c;
                        _idx_score = _idx_group + 4;
                        _idx_left = _idx_group + 5;
                        _idx_right = _idx_group + 6;
                    } else if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                        _idx_score = 4 * _h * _w + idx_i * _w + idx_j;
                        _idx_left = 5 * _h * _w + idx_i * _w + idx_j;
                        _idx_right = 6 * _h * _w + idx_i * _w + idx_j;
                    }
                    float score = _data[_idx_score];
                    bool is_left = _data[_idx_left] * score > _data[_idx_right] * score;
                    if (score > score_threshold) {
                        float _coord[_c];
                        if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                            for (int i = 0; i < 4; i++) {
                                _coord[i] = _data[_idx_group + i];
                            }
                        } else if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                            for (int i = 0; i < 4; i++) {
                                int _val_idx = i * _h * _w + idx_i * _w + idx_j;
                                _coord[i] = _data[_val_idx];
                            }
                        }

                        uint32_t grid_anchor_index = idx_i * _w + idx_j;
                        // xywh
                        DetectRect tmp;
                        tmp.w = std::pow(_coord[2] * 2, 2) * grid_anchor[grid_anchor_index].anchor_rw;
                        tmp.h = std::pow(_coord[3] * 2, 2) * grid_anchor[grid_anchor_index].anchor_rh;
                        tmp.x = (_coord[0] * 2 + grid_anchor[grid_anchor_index].grid_x) * grid_stride - tmp.w / 2;
                        tmp.y = (_coord[1] * 2 + grid_anchor[grid_anchor_index].grid_y) * grid_stride - tmp.h / 2;
                        tmp.confidence = score;
                        tmp.left_confidence = _data[_idx_left];
                        tmp.right_confidence = _data[_idx_right];
                        tmp.is_left = is_left;
                        tmp.nms_suppressed = false;
                        tmp_result.emplace_back(tmp);
                    }
                }
            }

            auto &lhand_rect = result.images_lhand_rects[batchn];
            auto &rhand_rect = result.images_rhand_rects[batchn];

            // nms
            nms(tmp_result, iou_threshold);

            // scale_coords
            int height, width;
            if (itensor_format == aisdk::xengine::TensorFormat::CHW) {
                height = itensor.m_tensors[multi_i].m_dims[1];
                width = itensor.m_tensors[multi_i].m_dims[2];
            } else if (itensor_format == aisdk::xengine::TensorFormat::HWC) {
                height = itensor.m_tensors[multi_i].m_dims[0];
                width = itensor.m_tensors[multi_i].m_dims[1];
            }

            float min_ratio =
                std::min(float(width) / float(origin_img_width), float(height) / float(origin_img_height));
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
                    AISDK_LOG_TRACE("push lhand rect: {}", iter.x);
                    lhand_rect.push_back(iter);
                } else if (false == iter.is_left && rhand_rect.size() == 0) {
                    AISDK_LOG_TRACE("push rhand rect: {}", iter.x);
                    rhand_rect.push_back(iter);
                }
            }
        }
    }
}

absl::Status HandDetectNet::Inference(const std::vector<Image> &baseinput, DetOutputInternal &baseresult) {
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
                AISDK_LOG_TRACE("HandDetectNet::Inference  Error!");
            }
        }
        return ret;
    } else {
        PreProcess(baseinput);
        absl::Status ret = m_net->RunNet();
        if (ret.ok()) {
            PostProcess(baseresult);

            AISDK_LOG_TRACE("[HandDetectNet::Inference] baseresult.images_lhand_rects[0].size(): {}",
                            baseresult.images_lhand_rects[0].size());
            AISDK_LOG_TRACE("[HandDetectNet::Inference] baseresult.images_lhand_rects[1].size(): {}",
                            baseresult.images_lhand_rects[1].size());
            AISDK_LOG_TRACE("[HandDetectNet::Inference] baseresult.images_rhand_rects[0].size(): {}",
                            baseresult.images_rhand_rects[0].size());
            AISDK_LOG_TRACE("[HandDetectNet::Inference] baseresult.images_rhand_rects[1].size(): {}",
                            baseresult.images_rhand_rects[1].size());
        } else {
            baseresult.images_lhand_rects.resize(otensor.m_batch);
            baseresult.images_rhand_rects.resize(otensor.m_batch);
            AISDK_LOG_TRACE("HandDetectNet::Inference  Error!");
        }
        return ret;
    }
}

absl::Status HandDetectNetv2::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                   aisdk::xengine::SessionConfig &session) {
    if (model.dont_batch && session.batch > 1) {
        session_batch = session.batch;
        net_batch1 = true;
        session.batch = 1;
        AISDK_LOG_TRACE("set net_batch1 true. session_batch[{}]", session_batch);
    }

    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }

    // 简单实现
    {
        itensor_format = checkshapeformat(model.vendor_type, itensor.m_tensors[0].m_rank);
        int height = 0;
        int width = 0;
        if (itensor_format == aisdk::xengine::TensorFormat::CHW) {
            height = itensor.m_tensors[0].m_dims[1];
            width = itensor.m_tensors[0].m_dims[2];
        } else if (itensor_format == aisdk::xengine::TensorFormat::HWC) {
            height = itensor.m_tensors[0].m_dims[0];
            width = itensor.m_tensors[0].m_dims[1];
        }

        grid_h = height / grid_stride;
        grid_w = width / grid_stride;
    }

    { otensor_format = checkshapeformat(model.vendor_type, otensor.m_tensors[0].m_rank); }

    grid_anchor.resize(grid_h * grid_w);
    for (auto i = 0; i < grid_h; i++) {
        for (auto j = 0; j < grid_w; j++) {
            grid_anchor[i * grid_w + j].grid_x = -0.5f + j * 1.0f;
            grid_anchor[i * grid_w + j].grid_y = -0.5f + i * 1.0f;
            grid_anchor[i * grid_w + j].anchor_rw = 33.0f;
            grid_anchor[i * grid_w + j].anchor_rh = 30.0f;
        }
    }

    auto &prof = aisdk::base::DebugProfiling::Get().GetOpt();
    export_netalgo_exec_info = prof.export_pipeline_exec_info_jsonstring;
    AISDK_LOG_TRACE("HandDetectNetv2::Inference  export_netalgo_exec_info={}", export_netalgo_exec_info);
    return ret;
}

absl::Status HandDetectNetv2::Inference(const std::vector<Image> &baseinput, DetOutputInternal &baseresult) {
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
                AISDK_LOG_TRACE("HandDetectNetv2::Inference  Error!");
            }
        }
        return ret;
    } else {
        PreProcess(baseinput);
        absl::Status ret = m_net->RunNet();
        if (ret.ok()) {
            PostProcess(baseresult);

            AISDK_LOG_TRACE("[HandDetectNetv2::Inference] baseresult.images_lhand_rects[0].size(): {}",
                            baseresult.images_lhand_rects[0].size());
            AISDK_LOG_TRACE("[HandDetectNetv2::Inference] baseresult.images_lhand_rects[1].size(): {}",
                            baseresult.images_lhand_rects[1].size());
            AISDK_LOG_TRACE("[HandDetectNetv2::Inference] baseresult.images_rhand_rects[0].size(): {}",
                            baseresult.images_rhand_rects[0].size());
            AISDK_LOG_TRACE("[HandDetectNetv2::Inference] baseresult.images_rhand_rects[1].size(): {}",
                            baseresult.images_rhand_rects[1].size());
        } else {
            baseresult.images_lhand_rects.resize(otensor.m_batch);
            baseresult.images_rhand_rects.resize(otensor.m_batch);
            AISDK_LOG_TRACE("HandDetectNetv2::Inference  Error!");
        }
        return ret;
    }
}

void HandDetectNetv2::PostProcess(DetOutputInternal &result) {
    // if (otensor.m_packed_bybatch == false) {
    //     return;
    // }

    AISDK_LOG_TRACE("HandDetectNetv2::PostProcess");

    result.images_lhand_rects.resize(otensor.m_batch);
    result.images_rhand_rects.resize(otensor.m_batch);

    int index_box = this->m_net->GetOutputTensorIndex("output_box");
    int index_cls = this->m_net->GetOutputTensorIndex("output_cls");

    int _h, _w, _c, element_byte;

    int box_c, box_h, box_w, cls_c, cls_h, cls_w, box_element_byte, cls_element_byte;

    for (int batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
        if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
            box_c = otensor.m_tensors[index_box].m_dims[0];
            box_h = otensor.m_tensors[index_box].m_dims[1];
            box_w = otensor.m_tensors[index_box].m_dims[2];
            cls_c = otensor.m_tensors[index_cls].m_dims[0];
            cls_h = otensor.m_tensors[index_cls].m_dims[1];
            cls_w = otensor.m_tensors[index_cls].m_dims[2];
        } else if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
            box_c = otensor.m_tensors[index_box].m_dims[2];
            box_h = otensor.m_tensors[index_box].m_dims[0];
            box_w = otensor.m_tensors[index_box].m_dims[1];
            cls_c = otensor.m_tensors[index_cls].m_dims[2];
            cls_h = otensor.m_tensors[index_cls].m_dims[0];
            cls_w = otensor.m_tensors[index_cls].m_dims[1];
        }

        // cls_w == box_w, cls_h == box_h
        _h = cls_h;
        _w = cls_w;

        box_element_byte = otensor.m_tensors[index_box].m_elementbyte;
        char *box_mem =
            (char *)otensor.m_tensors[index_box].m_viraddr + batch_i * box_h * box_w * box_c * box_element_byte;
        float *box_data = (float *)box_mem;

        cls_element_byte = otensor.m_tensors[index_cls].m_elementbyte;
        char *cls_mem =
            (char *)otensor.m_tensors[index_cls].m_viraddr + batch_i * cls_h * cls_w * cls_c * cls_element_byte;
        float *cls_data = (float *)cls_mem;

        // int box_num = box_c * box_h * box_w;
        // AISDK_LOG_TRACE("HandDetectNetv2::PostProcess, begin to output box info, box_num[{}]", box_num);
        // for (int i = 0; i < box_num; i++) {
        //     AISDK_LOG_TRACE("HandDetectNetv2::PostProcess, i[{}], box_data[{}]", i, box_data[i]);
        // }

        // AISDK_LOG_TRACE("HandDetectNetv2::PostProcess, end to output box info");

        // int cls_num = cls_c * cls_h * cls_w;
        // AISDK_LOG_TRACE("HandDetectNetv2::PostProcess, begin to output cls info, cls_num[{}]", cls_num);
        // for (int i = 0; i < cls_num; i++) {
        //     AISDK_LOG_TRACE("HandDetectNetv2::PostProcess, i[{}], cls_data[{}]", i, cls_data[i]);
        // }

        // AISDK_LOG_TRACE("HandDetectNetv2::PostProcess, end to output cls info");

        // if (box_c != FEATURE_BOX_NUM || cls_c != FEATURE_CLS_NUM || box_h !=
        // grid_h || box_w != grid_w ||
        //     cls_h != grid_h || cls_w != grid_w) {
        //     continue;
        // }

        AISDK_LOG_TRACE("HandDetectNetv2::Get Results");
        // clang-format off
        AISDK_LOG_TRACE(
            "HandDetectNetv2:: cls_c: {}, cls_h: {}, cls_w: {}, box_c: {}, box_h: {}, bow_w: {}",
            cls_c, cls_h, cls_w, box_c, box_h, box_w);
        // clang-format on

        std::vector<DetectRect> tmp_result;
        int cls_idx_group, cls_idx_score, cls_idx_left, cls_idx_right;
        int box_idx_group;
        for (int idx_i = 0; idx_i < _h; idx_i++) {      // y
            for (int idx_j = 0; idx_j < _w; idx_j++) {  // x
                if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                    cls_idx_group = idx_i * cls_w * cls_c + idx_j * cls_c;
                    cls_idx_score = cls_idx_group;
                    cls_idx_left = cls_idx_group + 1;
                    cls_idx_right = cls_idx_group + 2;
                } else if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                    cls_idx_score = 1 * cls_h * cls_w + idx_i * cls_w + idx_j;
                    cls_idx_left = 2 * cls_h * cls_w + idx_i * cls_w + idx_j;
                    cls_idx_right = 3 * cls_h * cls_w + idx_i * cls_w + idx_j;
                }
                float obj_score = cls_data[cls_idx_score];
                float left_score = cls_data[cls_idx_left] * obj_score;
                float right_score = cls_data[cls_idx_right] * obj_score;
                bool is_left = left_score > right_score;
                float score = is_left ? left_score : right_score;
                if (score > score_threshold) {
                    float _coord[box_c];
                    if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                        box_idx_group = idx_i * box_w * box_c + idx_j * box_c;
                        for (int i = 0; i < 4; i++) {
                            _coord[i] = box_data[box_idx_group + i];
                        }
                    } else if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                        for (int i = 0; i < 4; i++) {
                            int _val_idx = i * box_h * box_w + idx_i * box_w + idx_j;
                            _coord[i] = box_data[_val_idx];
                        }
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

        AISDK_LOG_TRACE("HandDetectNetv2::tmp_result size(before nms): {}", tmp_result.size());

        auto &lhand_rect = result.images_lhand_rects[batch_i];
        auto &rhand_rect = result.images_rhand_rects[batch_i];

        // nms
        nms(tmp_result, iou_threshold);

        AISDK_LOG_TRACE("HandDetectNetv2::tmp_result size(after nms): {}", tmp_result.size());

        // scale_coords
        int height, width;
        if (itensor_format == aisdk::xengine::TensorFormat::CHW) {
            height = itensor.m_tensors[0].m_dims[1];
            width = itensor.m_tensors[0].m_dims[2];
        } else if (itensor_format == aisdk::xengine::TensorFormat::HWC) {
            height = itensor.m_tensors[0].m_dims[0];
            width = itensor.m_tensors[0].m_dims[1];
        }

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

}  // namespace aisdk::algorithm
