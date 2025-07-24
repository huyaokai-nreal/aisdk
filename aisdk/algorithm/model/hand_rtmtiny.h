#pragma once

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <vector>
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "calculator_basenet.h"

namespace aisdk::algorithm {


class RTMTiny : public HandLandmarkBaseNet {
   public:
    RTMTiny()=default;
    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                aisdk::xengine::SessionConfig &session);
    void PreProcess(const std::vector<Image> &net_input);
    void PostProcess(Kpt2dResult &result);
    absl::StatusOr<Kpt2dResult> Inference(const std::vector<Image> &input) override;

   private:
    aisdk::xengine::TensorFormat itensor_format_;
    aisdk::xengine::TensorFormat otensor_format_;
    float img_mean = 114.4950;            // 图像预处理均值
    float img_std = 57.63;                // 图像预处理方差
    unsigned int input_shape_ = 128;
    unsigned int output_shape_ = 256;
    unsigned int keypoint_num_ = 21;
    float hand_label_cls_thr = 1.0;
    std::vector<float> mul_coeff_;
};

class RTMTinyFlora : public HandLandmarkBaseNet {
    public:
        RTMTinyFlora()=default;
        absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model,
                                    aisdk::xengine::SessionConfig &session);                // 初始化网络模型
        void PreProcess(const std::vector<Image> &net_input);                               // 前处理
        void PostProcess(Kpt2dResult &result);                                              // 后处理
        absl::StatusOr<Kpt2dResult> Inference(const std::vector<Image> &input) override;    // 模型推理
    
    private:
        // 模型相关配置
        aisdk::xengine::TensorFormat itensor_format_;    // input tensor内存布局
        aisdk::xengine::TensorFormat otensor_format_;    // output tensor内存布局
    
        // 热力图处理缓冲区
        std::vector<float> mul_coeff_;        // 热图坐标到原图坐标的缩放系数
    
        // 模型结构数据
        unsigned int input_shape_ = 128;      // 输入图像缓存（正方形，128*128像素）
        unsigned int output_shape_ = 256;     // 输出图像缓存（正方形, 32*32像素）
        unsigned int keypoint_num_ = 21;      // 手部关键点数量（21个关键点）
        float img_mean_ = 114.495;                     // 归一化使用的均值
        float img_std_ = 57.63;                        // 归一化使用的标准差
        float hand_label_cls_thr = 1.0;
    };
}  // namespace aisdk::algorithm
