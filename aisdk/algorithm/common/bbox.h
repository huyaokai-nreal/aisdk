#include "aisdk/base/type.h"
#include <Eigen/Dense>
namespace aisdk::algorithm {
    inline Vec4f_t bbox_xyxy2cs(const Vec4f_t& bbox, float padding = 1.0){
        Vec4f_t result = Vec4f_t::Zero();
        result.block<2,1>(0,0)  = (bbox.block<2,1>(0,0) + bbox.block<2, 1>(2, 0))/2;
        result.block<2,1>(2, 0) = (bbox.block<2,1>(2,0) - bbox.block<2,1>(0,0)) * padding;
        return result;
    }
    inline Vec4f_t bbox_cs2xyxy(const Vec4f_t& bbox, float padding = 1.0)
    {
        Vec2f_t new_scale = bbox.block<2,1>(2,0) / padding;
        Vec4f_t result = Vec4f_t::Zero();
        result.block<2,1>(0,0) = bbox.block<2,1>(0,0) - new_scale * 0.5;
        result.block<2,1>(2,0) = bbox.block<2,1>(2,0) + new_scale * 0.5;
        return result;
    }

    inline Vec4f_t bbox_xywh2cs(const Vec4f_t& bbox) {
        Vec4f_t result = bbox;
        result.block<2,1>(0, 0) = bbox.block<2,1>(0, 0) + bbox.block<2,1>(2,0)*0.5;
        return result;
    }
}