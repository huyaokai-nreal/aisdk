#pragma  once 
#include <Eigen/Dense>
#include "aisdk/base/camera_model.h"
#include "aisdk/base/type.h"
namespace aisdk::algorithm {
    std::shared_ptr<base::PerspectiveCameraModel> GetVirtualCameraFromBox(base::BaseCameraModel* ori_camera, const Vec4f_t& bbox_cs,
                                                 const Vec2f_t& new_image_size);

}