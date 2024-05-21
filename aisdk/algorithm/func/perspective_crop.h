#pragma  once 
#include <Eigen/Dense>
#include "aisdk/base/camera_model.h"
#include "aisdk/base/type.h"
#include <memory>
namespace aisdk::algorithm {
    std::shared_ptr<base::PerspectiveCameraModel> GetVirtualCameraFromBox(base::BaseCameraModel* camera_orig, const Vec4f_t& bbox_cs,
                                                 const Vec2f_t& new_image_size);

}