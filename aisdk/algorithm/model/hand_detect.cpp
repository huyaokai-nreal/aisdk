#include "hand_detect.h"

#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/base/profiling.h"
#include "aisdk/xengine/cv/xr_cv.h"

namespace aisdk::algorithm {

/// @brief 初始化手部检测网络
/// @param algo 算法配置参数（后处理参数等）
/// @param model 模型配置（模型路径、输入输出规格等）
/// @param session 会话配置（批量大小、设备选择等）
/// @return absl::Status 返回初始化状态
absl::Status HandDetectNet::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                 aisdk::xengine::SessionConfig &session) {
    // step1: 批量处理策略配置（兼容不支持批处理的模型）
    if (model.dont_batch && session.batch > 1) {
        session_batch = session.batch;
        net_batch1 = true;
        session.batch = 1;
    }

    // step2: 调用基类初始化方法，完成网络加载等公共操作
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }

    // step3: 解析输入张量格式并计算网格尺寸(基于厂商类型和张量维度确定数据格式(CHW/HWC))
    itensor_format = checkshapeformat(model.vendor_type, itensor.m_tensors[0].m_rank);

    // 根据格式解析输入尺寸
    int height = 0;
    int width = 0;
    if (itensor_format == aisdk::xengine::TensorFormat::CHW) {
        height = itensor.m_tensors[0].m_dims[1];
        width = itensor.m_tensors[0].m_dims[2];
    } else if (itensor_format == aisdk::xengine::TensorFormat::HWC) {
        height = itensor.m_tensors[0].m_dims[0];
        width = itensor.m_tensors[0].m_dims[1];
    } else {
        AISDK_LOG_ERROR("HandDetectNet::Init itensor_format[{}] is illegal. should be CHW[{}] or HWC[{}]",
                        static_cast<int>(itensor_format), static_cast<int>(aisdk::xengine::TensorFormat::CHW),
                        static_cast<int>(aisdk::xengine::TensorFormat::HWC));
        return {absl::StatusCode::kInternal, "[HandDetectNetv2::Init] itensor_format is illegal."};
    }

    // 计算特征网格尺寸（输入尺寸/网格步长）
    grid_h = height / grid_stride;
    grid_w = width / grid_stride;

    // step4: 解析输出张量格式
    otensor_format = checkshapeformat(model.vendor_type, otensor.m_tensors[0].m_rank);

    // step5: 预分配锚点容器空间（网格总数 = 行数×列数）
    grid_anchor.resize(grid_h * grid_w);

    // step6: 生成网格锚点系统
    for (auto i = 0; i < grid_h; i++) {
        for (auto j = 0; j < grid_w; j++) {
            // 计算网格中心坐标（归一化坐标，原点在图像中心）
            grid_anchor[i * grid_w + j].grid_x = -0.5f + j * 1.0f;  // X轴中心位置
            grid_anchor[i * grid_w + j].grid_y = -0.5f + i * 1.0f;  // Y轴中心位置

            // 设置锚点基准尺寸（基于典型手部尺寸）
            grid_anchor[i * grid_w + j].anchor_rw = 33.0f;  // 参考宽度（像素）
            grid_anchor[i * grid_w + j].anchor_rh = 30.0f;  // 参考高度（像素）
        }
    }

    // step7: 性能分析配置
    auto &prof = aisdk::base::DebugProfiling::Get().GetOpt();
    export_netalgo_exec_info = prof.export_pipeline_exec_info_jsonstring;
    AISDK_LOG_TRACE("HandDetectNet::Init export_netalgo_exec_info={}", export_netalgo_exec_info);
    return ret;
}

/// @brief 预处理输入图像，准备手部检测网络的输入数据
/// @param net_input 包含输入图像信息的向量
void HandDetectNet::PreProcess(const std::vector<Image> &net_input) {
    // step1: 检查输入数量与模型配置是否匹配
    int ai = itensor.m_batch * itensor.m_multishape_num;  // 模型预期输入数量 = 批次大小 × 多形状数
    int bi = net_input.size();                            // 实际输入图像数量

    // 输入验证：输入数量必须匹配且模型已配置批处理模式
    if (ai != bi || itensor.m_packed_bybatch == false) {
        AISDK_LOG_ERROR(
            "HandDetectNet::PreProcess ai[{}] not equal to bi[{}] or m_apcked_bybatch[{}] is false, do not do "
            "preprocess. itensor.m_batch[{}], "
            "itensor.m_multishape_num[{}]",
            ai, bi, itensor.m_packed_bybatch, itensor.m_batch, itensor.m_multishape_num);
        return;
    }

    // step2: 遍历处理每张输入图像
    for (int i = 0; i < bi; i++) {
        // step3: 获取当前输入图像的OpenCV矩阵
        auto &img = net_input[i].m_mat;

        // step4: 计算张量索引位置
        int multi_i = i / itensor.m_batch;  // 多形状索引（支持多种输入形状）
        int batch_i = i % itensor.m_batch;  // 批次索引（当前批次中的位置）

        // step5: 根据张量格式解析输入尺寸
        int height = 0;
        int width = 0;
        int channels = 0;
        if (itensor_format == aisdk::xengine::TensorFormat::CHW) {
            channels = itensor.m_tensors[multi_i].m_dims[0];
            height = itensor.m_tensors[multi_i].m_dims[1];
            width = itensor.m_tensors[multi_i].m_dims[2];
        } else if (itensor_format == aisdk::xengine::TensorFormat::HWC) {
            height = itensor.m_tensors[multi_i].m_dims[0];
            width = itensor.m_tensors[multi_i].m_dims[1];
            channels = itensor.m_tensors[multi_i].m_dims[2];
        } else {
            // do nothing
        }

        // step6: 准备张量内存信息
        int element_byte = itensor.m_tensors[multi_i].m_elementbyte;  // 张量元素字节大小
        int mem_size = height * width * channels * element_byte;      // 单张输入所需内存大小
        char *mem = (char *)itensor.m_tensors[multi_i].m_viraddr + batch_i * mem_size;  // 当前输入的目标内存地址

        // step7: 设置归一化参数（标准预处理配置）
        float _mean = 0.0f;    // 均值（不做均值减法）
        float _norm = 255.0f;  // 缩放因子（像素值除以255）

        // step8: 记录原始图像尺寸（用于后处理阶段坐标映射）
        origin_img_width = img.cols;   // 原始图像宽度
        origin_img_height = img.rows;  // 原始图像高度

        // step9: 计算宽高缩放比例
        float wratio = float(width) / float(origin_img_width);    // 宽度缩放比例
        float hratio = float(height) / float(origin_img_height);  // 高度缩放比例

        // ===================== 平台特化处理路径 =====================
        // 在Android ARM64平台上使用优化缩放（特定缩放比例0.4，即2.5倍缩小）
#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
        if ((wratio - 0.4f) < 1e-5 && (hratio - 0.4f) < 1e-5) {
            // step10: 创建目标内存包装矩阵
            cv::Mat image_resized(cv::Size(width, height), CV_32FC1, mem);

            // 调用ARM NEON优化的快速缩放函数(仅支持等比例缩小2.5倍)
            aisdk::xengine::NrResize((unsigned char *)img.data, (float *)image_resized.data, height, width, 1.f, _mean,
                                     _norm);
        } else
#endif
        // ===================== 通用处理路径 =====================
        {
            // step11: 计算最佳缩放比例并选择插值方法
            float ratio = std::min(wratio, hratio);                        // 取最小缩放比例（保持宽高比）
            int tmp = (ratio < 1.0f) ? cv::INTER_AREA : cv::INTER_LINEAR;  // 缩小用区域插值，放大用线性插值

            // step12: 缩放图像到模型输入尺寸
            cv::Mat image_resized(cv::Size(width, height), CV_8UC1);  // 创建临时8位图像
            cv::resize(img, image_resized, cv::Size(width, height), 0, 0, tmp);

            // step13: 类型转换与归一化处理
            image_resized.convertTo(image_resized, CV_32FC1);         // 转换为32位浮点数
            cv::Mat new_mat(cv::Size(width, height), CV_32FC1, mem);  // 创建目标内存包装矩阵

            // 执行归一化：(像素值 - 均值) / 缩放因子
            new_mat = (image_resized - _mean) / _norm;
        }
    }
}

/**
 * @brief 手势检测网络的后处理函数，解析网络输出并得到左右手检测框
 *
 * 本函数负责解析手势检测网络的输出张量，将原始输出转换为检测到的左右手矩形框信息，
 * 并应用非极大值抑制(NMS)和坐标变换，最终输出左右手的检测结果。
 *
 * @param result 输出结果结构体，包含左右手检测框列表
 */
void HandDetectNet::PostProcess(DetOutputInternal &result) {
    // step1: 检查输出张量是否按批次封装（要求按批次组织数据）
    if (otensor.m_packed_bybatch == false) {
        return;
    }

    // 初始化输出结果的左右手矩形框容器
    result.images_lhand_rects.resize(otensor.m_batch);  // 左手检测框列表
    result.images_rhand_rects.resize(otensor.m_batch);  // 右手检测框列表

    // 循环处理多尺度输出（某些网络可能输出多个尺度的检测结果）
    for (int multi_i = 0; multi_i < otensor.m_multishape_num; multi_i++) {
        // 对每个批次的图像进行处理
        for (int batch_i = 0; batch_i < otensor.m_batch; batch_i++) {
            int _h = 0;
            int _w = 0;
            int _c = 0;
            int element_byte = 0;  // 每个元素的字节大小

            // 根据输出张量格式确定维度顺序
            if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                _c = otensor.m_tensors[multi_i].m_dims[0];
                _h = otensor.m_tensors[multi_i].m_dims[1];
                _w = otensor.m_tensors[multi_i].m_dims[2];
            } else if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                _h = otensor.m_tensors[multi_i].m_dims[0];
                _w = otensor.m_tensors[multi_i].m_dims[1];
                _c = otensor.m_tensors[multi_i].m_dims[2];
            } else {
                // do nothing
            }

            // 计算当前批次在当前输出尺度下的内存起始位置
            element_byte = otensor.m_tensors[multi_i].m_elementbyte;
            char *mem = (char *)otensor.m_tensors[multi_i].m_viraddr + batch_i * _h * _w * _c * element_byte;
            float *_data = (float *)mem;

            // 检查特征图维度是否匹配预期
            if (_c != FEATURE_NUM || _h != grid_h || _w != grid_w) {
                continue;
            }

            std::vector<DetectRect> tmp_result;  // 临时存储所有检测框

            // 遍历特征图上的每个位置（每个网格单元）
            for (int idx_i = 0; idx_i < _h; idx_i++) {      // y坐标遍历
                for (int idx_j = 0; idx_j < _w; idx_j++) {  // x坐标遍历
                    int _idx_group = 0;                     // 不同属性的索引位置
                    int _idx_score = 0;
                    int _idx_left = 0;
                    int _idx_right = 0;

                    // 根据张量格式计算不同属性的索引位置
                    if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                        _idx_group = idx_i * _w * _c + idx_j * _c;
                        _idx_score = _idx_group + 4;  // 置信度分数位置
                        _idx_left = _idx_group + 5;   // 左手置信度位置
                        _idx_right = _idx_group + 6;  // 右手置信度位置
                    } else if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                        _idx_score = 4 * _h * _w + idx_i * _w + idx_j;  // 置信度分数位置
                        _idx_left = 5 * _h * _w + idx_i * _w + idx_j;   // 左手置信度位置
                        _idx_right = 6 * _h * _w + idx_i * _w + idx_j;  // 右手置信度位置
                    } else {
                        // do nothing
                    }

                    // 获取当前网格单元的置信度分数
                    float score = _data[_idx_score];

                    // 判断是左手还是右手（根据左右手置信度的加权分数）
                    bool is_left = _data[_idx_left] * score > _data[_idx_right] * score;
                    if (score > score_threshold) {
                        float _coord[_c];

                        // 提取坐标信息（根据张量格式）
                        if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                            for (int i = 0; i < 4; i++) {
                                _coord[i] = _data[_idx_group + i];
                            }
                        } else if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                            for (int i = 0; i < 4; i++) {
                                int _val_idx = i * _h * _w + idx_i * _w + idx_j;
                                _coord[i] = _data[_val_idx];
                            }
                        } else {
                            // do nothing
                        }

                        // 计算当前网格在特征图中的索引
                        uint32_t grid_anchor_index = idx_i * _w + idx_j;

                        /**
                         * 创建检测矩形对象并填充信息(xywh)
                         * 计算宽度(w)：公式源于网络设计 (pow(coord[2]*2, 2) * 锚点基准宽)
                         * 计算高度(h)：公式源于网络设计 (pow(coord[3]*2, 2) * 锚点基准高)
                         * 计算中心点x坐标：(coord[0]*2 + 网格x位置) * 网格步长 - 宽度/2
                         * 计算中心点y坐标：(coord[1]*2 + 网格y位置) * 网格步长 - 高度/2
                         */
                        DetectRect tmp;
                        tmp.w = std::pow(_coord[2] * 2, 2) * grid_anchor[grid_anchor_index].anchor_rw;
                        tmp.h = std::pow(_coord[3] * 2, 2) * grid_anchor[grid_anchor_index].anchor_rh;
                        tmp.x = (_coord[0] * 2 + grid_anchor[grid_anchor_index].grid_x) * grid_stride - tmp.w / 2;
                        tmp.y = (_coord[1] * 2 + grid_anchor[grid_anchor_index].grid_y) * grid_stride - tmp.h / 2;

                        // 填充其他属性
                        tmp.confidence = score;                    // 置信度分数
                        tmp.left_confidence = _data[_idx_left];    // 左手置信度
                        tmp.right_confidence = _data[_idx_right];  // 右手置信度
                        tmp.is_left = is_left;                     // 是否是左手
                        tmp.nms_suppressed = false;                // NMS标记初始化为未抑制
                        tmp_result.emplace_back(tmp);
                    }
                }
            }

            AISDK_LOG_TRACE("tmp_result.size(): {}", tmp_result.size());

            // 获取当前批次的左右手检测结果引用
            auto &lhand_rect = result.images_lhand_rects[batch_i];
            auto &rhand_rect = result.images_rhand_rects[batch_i];

            // 应用非极大值抑制(NMS)，消除重叠框
            nms(tmp_result, iou_threshold);

            // 获取输入张量的尺寸（用于坐标变换）
            int height = 0;
            int width = 0;
            if (itensor_format == aisdk::xengine::TensorFormat::CHW) {
                height = itensor.m_tensors[multi_i].m_dims[1];
                width = itensor.m_tensors[multi_i].m_dims[2];
            } else if (itensor_format == aisdk::xengine::TensorFormat::HWC) {
                height = itensor.m_tensors[multi_i].m_dims[0];
                width = itensor.m_tensors[multi_i].m_dims[1];
            } else {
                // do nothing
            }

            // 计算输入张量与原始图像的缩放比例和填充
            float min_ratio =
                std::min(float(width) / float(origin_img_width), float(height) / float(origin_img_height));
            float padx = (float(width) - float(origin_img_width) * min_ratio) / 2;
            float pady = (float(height) - float(origin_img_height) * min_ratio) / 2;

            // 遍历经过NMS后的检测框
            for (auto &iter : tmp_result) {
                if (iter.nms_suppressed) {
                    continue;
                }

                // 将框坐标从网络输入尺寸转换回原始图像尺寸
                iter.x = std::round((iter.x - padx) / min_ratio);
                iter.y = std::round((iter.y - pady) / min_ratio);
                iter.w = std::round(iter.w / min_ratio);
                iter.h = std::round(iter.h / min_ratio);

                // 将检测框分类到左手或右手容器
                if (true == iter.is_left && lhand_rect.size() == 0) {
                    AISDK_LOG_TRACE("push lhand rect: x[{}], y[{}], w[{}], h[{}]", iter.x, iter.y, iter.w, iter.h);
                    lhand_rect.push_back(iter);
                } else if (false == iter.is_left && rhand_rect.size() == 0) {
                    AISDK_LOG_TRACE("push rhand rect: x[{}], y[{}], w[{}], h[{}]", iter.x, iter.y, iter.w, iter.h);
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

/// @brief 执行手部检测网络推理
/// @param baseinput 输入图像向量
/// @param baseresult 输出检测结果结构体
/// @return absl::Status 推理状态（成功或错误信息）
absl::Status HandDetectNet::Inference(const std::vector<Image> &baseinput, DetOutputInternal &baseresult) {
    if (net_batch1) {  // 模型不支持批量推理处理
        // 初始化状态和结果容器
        absl::Status ret;
        baseresult.images_lhand_rects.resize(session_batch);  // 初始化左手检测结果容器
        baseresult.images_rhand_rects.resize(session_batch);  // 初始化右手检测结果容器

        // 逐批处理每张图像
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
    } else {  // 模型指出批量推理处理
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

/// @brief 初始化手部检测网络
/// @param algo 算法配置参数（后处理参数等）
/// @param model 模型配置（模型路径、输入输出规格等）
/// @param session 会话配置（批量大小、设备选择等）
/// @return absl::Status 返回初始化状态
absl::Status HandDetectNetv2::Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                   aisdk::xengine::SessionConfig &session) {
    // step1: 批量处理策略配置（兼容不支持批处理的模型）
    if (model.dont_batch && session.batch > 1) {
        session_batch = session.batch;
        net_batch1 = true;
        session.batch = 1;
        AISDK_LOG_TRACE("set net_batch1 true. session_batch[{}]", session_batch);
    }

    // step2: 调用基类初始化方法，完成网络加载等公共操作
    auto ret = CalculatorBaseNet::Init(algo, model, session);
    if (!ret.ok()) {
        return ret;
    }

    // step3: 解析输入张量格式并计算网格尺寸(基于厂商类型和张量维度确定数据格式(CHW/HWC))
    itensor_format = checkshapeformat(model.vendor_type, itensor.m_tensors[0].m_rank);

    // 根据格式解析输入尺寸
    int height = 0;
    int width = 0;
    if (itensor_format == aisdk::xengine::TensorFormat::CHW) {
        height = itensor.m_tensors[0].m_dims[1];
        width = itensor.m_tensors[0].m_dims[2];
    } else if (itensor_format == aisdk::xengine::TensorFormat::HWC) {
        height = itensor.m_tensors[0].m_dims[0];
        width = itensor.m_tensors[0].m_dims[1];
    } else {
        AISDK_LOG_ERROR("HandDetectNetv2::Init itensor_format[{}] is illegal. should be CHW[{}] or HWC[{}]",
                        static_cast<int>(itensor_format), static_cast<int>(aisdk::xengine::TensorFormat::CHW),
                        static_cast<int>(aisdk::xengine::TensorFormat::HWC));
        return {absl::StatusCode::kInternal, "[HandDetectNetv2::Init] itensor_format is illegal."};
    }

    // 计算特征网格尺寸（输入尺寸/网格步长）
    grid_h = height / grid_stride;
    grid_w = width / grid_stride;

    // step4: 解析输出张量格式
    otensor_format = checkshapeformat(model.vendor_type, otensor.m_tensors[0].m_rank);

    // step5: 预分配锚点容器空间（网格总数 = 行数×列数）
    grid_anchor.resize(grid_h * grid_w);

    // step6: 生成网格锚点系统
    for (auto i = 0; i < grid_h; i++) {
        for (auto j = 0; j < grid_w; j++) {
            // 计算网格中心坐标（归一化坐标，原点在图像中心）
            grid_anchor[i * grid_w + j].grid_x = -0.5f + j * 1.0f;  // X轴中心位置
            grid_anchor[i * grid_w + j].grid_y = -0.5f + i * 1.0f;  // Y轴中心位置

            // 设置锚点基准尺寸（基于典型手部尺寸）
            grid_anchor[i * grid_w + j].anchor_rw = 33.0f;  // 参考宽度（像素）
            grid_anchor[i * grid_w + j].anchor_rh = 30.0f;  // 参考高度（像素）
        }
    }

    // step7: 性能分析配置
    auto &prof = aisdk::base::DebugProfiling::Get().GetOpt();
    export_netalgo_exec_info = prof.export_pipeline_exec_info_jsonstring;
    AISDK_LOG_TRACE("HandDetectNetv2::Init export_netalgo_exec_info={}", export_netalgo_exec_info);
    return ret;
}

/// @brief 执行手部检测网络推理
/// @param baseinput 输入图像向量
/// @param baseresult 输出检测结果结构体
/// @return absl::Status 推理状态（成功或错误信息）
absl::Status HandDetectNetv2::Inference(const std::vector<Image> &baseinput, DetOutputInternal &baseresult) {
    if (net_batch1) {  // 模型不支持批量推理处理
        // 初始化状态和结果容器
        absl::Status ret;
        baseresult.images_lhand_rects.resize(session_batch);  // 初始化左手检测结果容器
        baseresult.images_rhand_rects.resize(session_batch);  // 初始化右手检测结果容器

        // 逐批处理每张图像
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
    } else {  // 模型支持批量推理处理
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

/// @brief 执行手部检测网络的后处理（v2版本）
///
/// 该函数负责解析网络的多个输出分支（框回归和分类），完成以下处理：
/// 1. 读取"output_box"和"output_cls"两个输出张量
/// 2. 生成初步检测结果
/// 3. 应用非极大值抑制（NMS）过滤重叠框
/// 4. 将坐标从网络输入空间转换到原始图像空间
/// 5. 将结果分别归类到左右手容器
///
/// @param result 输出结果结构体，用于存储处理后的检测框
void HandDetectNetv2::PostProcess(DetOutputInternal &result) {
    // if (otensor.m_packed_bybatch == false) {
    //     return;
    // }

    AISDK_LOG_TRACE("HandDetectNetv2::PostProcess");

    // step1: 初始化结果容器
    result.images_lhand_rects.resize(otensor.m_batch);
    result.images_rhand_rects.resize(otensor.m_batch);

    // step2: 获取输出张量索引
    int index_box = this->m_net->GetOutputTensorIndex("output_box");
    int index_cls = this->m_net->GetOutputTensorIndex("output_cls");

    int box_c = 0;
    int box_h = 0;
    int box_w = 0;
    int cls_c = 0;
    int cls_h = 0;
    int cls_w = 0;

    // step3: 批次循环处理
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
        } else {
            // do nothing
        }

        // step4: 计算当前输出尺度下的内存起始位置，并获取数据内容到box_data和cls_data中
        int box_element_byte = otensor.m_tensors[index_box].m_elementbyte;
        char *box_mem =
            (char *)otensor.m_tensors[index_box].m_viraddr + batch_i * box_h * box_w * box_c * box_element_byte;
        float *box_data = (float *)box_mem;

        int cls_element_byte = otensor.m_tensors[index_cls].m_elementbyte;
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
        AISDK_LOG_TRACE("HandDetectNetv2:: cls_c[{}], cls_h[{}], cls_w[{}], box_c[{}], box_h[{}], bow_w[{}]", cls_c,
                        cls_h, cls_w, box_c, box_h, box_w);

        // step5: 特征图遍历，遍历所有网格单元，获取合理结果并保存到tmp_result中
        std::vector<DetectRect> tmp_result;                // 临时存储所有检测框
        for (int idx_i = 0; idx_i < cls_h; idx_i++) {      // y坐标遍历
            for (int idx_j = 0; idx_j < cls_w; idx_j++) {  // x坐标遍历
                int cls_idx_group = 0;                     // 不同属性的索引位置
                int cls_idx_score = 0;
                int cls_idx_left = 0;
                int cls_idx_right = 0;

                // 根据张量格式计算不同属性的索引位置
                if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                    cls_idx_group = idx_i * cls_w * cls_c + idx_j * cls_c;
                    cls_idx_score = cls_idx_group;      // 置信度分数位置
                    cls_idx_left = cls_idx_group + 1;   // 左手置信度位置
                    cls_idx_right = cls_idx_group + 2;  // 右手置信度位置
                } else if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                    cls_idx_score = 1 * cls_h * cls_w + idx_i * cls_w + idx_j;  // 置信度分数位置
                    cls_idx_left = 2 * cls_h * cls_w + idx_i * cls_w + idx_j;   // 左手置信度位置
                    cls_idx_right = 3 * cls_h * cls_w + idx_i * cls_w + idx_j;  // 右手置信度位置
                } else {
                    // do nothing
                }

                // 当前网格单元的置信度分数
                float obj_score = cls_data[cls_idx_score];
                float left_score = cls_data[cls_idx_left] * obj_score;
                float right_score = cls_data[cls_idx_right] * obj_score;

                // 判断是左手还是右手（根据左右手置信度的加权分数）
                bool is_left = left_score > right_score;
                float score = is_left ? left_score : right_score;

                // 如果置信度超过阈值，则处理该检测结果
                if (score > score_threshold) {
                    float _coord[box_c];

                    // 进行这组结果的坐标提取
                    if (otensor_format == aisdk::xengine::TensorFormat::HWC) {
                        int box_idx_group = idx_i * box_w * box_c + idx_j * box_c;
                        for (int i = 0; i < 4; i++) {
                            _coord[i] = box_data[box_idx_group + i];
                        }
                    } else if (otensor_format == aisdk::xengine::TensorFormat::CHW) {
                        for (int i = 0; i < 4; i++) {
                            int _val_idx = i * box_h * box_w + idx_i * box_w + idx_j;
                            _coord[i] = box_data[_val_idx];
                        }
                    } else {
                        // do nothing
                    }

                    // 计算当前网格在特征图中的索引
                    uint32_t grid_anchor_index = idx_i * box_w + idx_j;

                    /**
                     * 创建检测矩形对象并填充信息(xywh)
                     * 计算宽度(w)：公式源于网络设计 (pow(coord[2]*2, 2) * 锚点基准宽)
                     * 计算高度(h)：公式源于网络设计 (pow(coord[3]*2, 2) * 锚点基准高)
                     * 计算中心点x坐标：(coord[0]*2 + 网格x位置) * 网格步长 - 宽度/2
                     * 计算中心点y坐标：(coord[1]*2 + 网格y位置) * 网格步长 - 高度/2
                     */
                    DetectRect tmp;
                    tmp.w = std::pow(_coord[2] * 2, 2) * grid_anchor[grid_anchor_index].anchor_rw;
                    tmp.h = std::pow(_coord[3] * 2, 2) * grid_anchor[grid_anchor_index].anchor_rh;
                    tmp.x = (_coord[0] * 2 + grid_anchor[grid_anchor_index].grid_x) * grid_stride - tmp.w / 2;
                    tmp.y = (_coord[1] * 2 + grid_anchor[grid_anchor_index].grid_y) * grid_stride - tmp.h / 2;

                    // 其他属性信息
                    tmp.confidence = score;                          // 置信度分数
                    tmp.left_confidence = cls_data[cls_idx_left];    // 左手置信度
                    tmp.right_confidence = cls_data[cls_idx_right];  // 右手置信度
                    tmp.is_left = is_left;                           // 是否左右手
                    tmp.nms_suppressed = false;                      // NMS标记初始化为未抑制
                    tmp_result.emplace_back(tmp);
                }
            }
        }

        AISDK_LOG_TRACE("HandDetectNetv2::tmp_result size(before nms): {}", tmp_result.size());

        auto &lhand_rect = result.images_lhand_rects[batch_i];
        auto &rhand_rect = result.images_rhand_rects[batch_i];

        // step6: 对获取的临时结果，应用非极大值抑制(NMS)，消除重叠框
        nms(tmp_result, iou_threshold);

        AISDK_LOG_TRACE("HandDetectNetv2::tmp_result size(after nms): {}", tmp_result.size());

        // step7: 获取不同输入格式下的height和width，并进一步计算输入张量与原始图像的缩放比例和填充
        int height = 0;
        int width = 0;
        if (itensor_format == aisdk::xengine::TensorFormat::CHW) {
            height = itensor.m_tensors[0].m_dims[1];
            width = itensor.m_tensors[0].m_dims[2];
        } else if (itensor_format == aisdk::xengine::TensorFormat::HWC) {
            height = itensor.m_tensors[0].m_dims[0];
            width = itensor.m_tensors[0].m_dims[1];
        } else {
            // do nothing
        }

        // 计算输入张量和原始图像的缩放比例和填充
        float min_ratio = std::min(float(width) / float(origin_img_width), float(height) / float(origin_img_height));
        float padx = (float(width) - float(origin_img_width) * min_ratio) / 2;
        float pady = (float(height) - float(origin_img_height) * min_ratio) / 2;

        // step8: 遍历经过NMS后的检测框，获取一组最终左右手的结果，填充到lhand_rect和rhand_rect中
        for (auto &iter : tmp_result) {
            if (iter.nms_suppressed) {
                continue;
            }

            // 将框坐标从网络输入尺寸转换回原始图像尺寸
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
            } else {
                // do nothing
            }
        }
    }
}

}  // namespace aisdk::algorithm
