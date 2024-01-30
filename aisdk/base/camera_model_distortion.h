#pragma once
#include <Eigen/Dense>
#include <vector>
namespace aisdk {
class Distortion {
   public:
    virtual std::vector<Eigen::Vector2f> evaluate(const std::vector<Eigen::Vector2f>& point_2d) = 0;
    virtual ~Distortion() = default;
};

class NoDistortion : Distortion {
   public:
    virtual std::vector<Eigen::Vector2f> evaluate(const std::vector<Eigen::Vector2f>& point_2d) { return point_2d; };
};

class OpenCVPinholeCameraDistortion : Distortion {
   public:
    OpenCVPinholeCameraDistortion(float k1, float k2, float p1, float p2, float k3)
        : k1_(k1), k2_(k2), p1_(p1), p2_(p2), k3_(k3) {}

    OpenCVPinholeCameraDistortion() = delete;
    std::vector<Eigen::Vector2f> evaluate(const std::vector<Eigen::Vector2f>& point_2d);
    std::vector<float> getDistortionParams() const { return {k1_, k2_, p1_, p2_, k3_}; }

   private:
    float k1_;
    float k2_;
    float p1_;
    float p2_;
    float k3_;
};

class OpenCVFisheyeCameraDistortion {
   public:
    OpenCVFisheyeCameraDistortion(float k1, float k2, float k3, float k4) : k1_(k1), k2_(k2), k3_(k3), k4_(k4) {}
    OpenCVFisheyeCameraDistortion() = delete;
    std::vector<Eigen::Vector2f> evaluate(const std::vector<Eigen::Vector2f>& point_2d);

    std::vector<float> getDistortionParams() const { return {k1_, k2_, k3_, k4_}; }

   private:
    float k1_;
    float k2_;
    float k3_;
    float k4_;
};

class Fisheye62CameraDistortion {
   public:
    Fisheye62CameraDistortion(float k1, float k2, float k3, float k4, float p1, float p2, float k5, float k6)
        : k1_(k1), k2_(k2), k3_(k3), k4_(k4), p1_(p1), p2_(p2), k5_(k5), k6_(k6) {}

    Fisheye62CameraDistortion() = delete;
    std::vector<Eigen::Vector2f> evaluate(const std::vector<Eigen::Vector2f>& point_2d);
    std::vector<float> getDistortionParams() { return {k1_, k2_, k3_, k4_, p1_, p2_, k5_, k6_}; }

   private:
    float k1_;
    float k2_;
    float k3_;
    float k4_;
    float p1_;
    float p2_;
    float k5_;
    float k6_;
};

}  // namespace aisdk