#pragma once
#include <Eigen/Dense>
#include <vector>
namespace aisdk::base {
class Distortion {
   public:
    virtual std::vector<Eigen::Vector2f> distort(const std::vector<Eigen::Vector2f>& point_2d) = 0;
    virtual std::vector<Eigen::Vector2f> undistort(const std::vector<Eigen::Vector2f>& point_2d) = 0;
    virtual ~Distortion() = default;
};

class NoDistortion : public Distortion {
   public:
     std::vector<Eigen::Vector2f> distort(const std::vector<Eigen::Vector2f>& point_2d) override { return point_2d; };
     std::vector<Eigen::Vector2f> undistort(const std::vector<Eigen::Vector2f>& point_2d) override { return point_2d; };
};

class OpenCVPinholeCameraDistortion : public Distortion {
   public:
    OpenCVPinholeCameraDistortion(float k1, float k2, float p1, float p2, float k3)
        : k1_(k1), k2_(k2), p1_(p1), p2_(p2), k3_(k3) {}

    OpenCVPinholeCameraDistortion() = delete;
    std::vector<Eigen::Vector2f> distort(const std::vector<Eigen::Vector2f>& point_2d) override;
    std::vector<Eigen::Vector2f> undistort(const std::vector<Eigen::Vector2f>& point_2d) override;
    std::vector<float> getDistortionParams() const { return {k1_, k2_, p1_, p2_, k3_}; }

   private:
    float k1_;
    float k2_;
    float p1_;
    float p2_;
    float k3_;
};

class OpenCVFisheyeCameraDistortion : public Distortion{
   public:
    OpenCVFisheyeCameraDistortion(float k1, float k2, float k3, float k4) : k1_(k1), k2_(k2), k3_(k3), k4_(k4) {}
    OpenCVFisheyeCameraDistortion() = delete;
    std::vector<Eigen::Vector2f> distort(const std::vector<Eigen::Vector2f>& point_2d) override;
    std::vector<Eigen::Vector2f> undistort(const std::vector<Eigen::Vector2f>& point_2d) override;

    std::vector<float> getDistortionParams() const { return {k1_, k2_, k3_, k4_}; }

   private:
    float k1_;
    float k2_;
    float k3_;
    float k4_;
};
class Fisheye624CameraDistortion: public Distortion {
   public:
    Fisheye624CameraDistortion(float k1, float k2, float k3, float k4, float p1, float p2, float k5, float k6, float s1,
                               float s2, float s3, float s4)
        : k1_(k1), k2_(k2), k3_(k3), k4_(k4), k5_(k5), k6_(k6), p1_(p1), p2_(p2), s1_(s1), s2_(s2), s3_(s3), s4_(s4) {}

    Fisheye624CameraDistortion() = delete;
    std::vector<Eigen::Vector2f> distort(const std::vector<Eigen::Vector2f>& point_2d) override;
    std::vector<Eigen::Vector2f> undistort(const std::vector<Eigen::Vector2f>& point_2d) override;
    std::vector<float> getDistortionParams() { return {k1_, k2_, k3_, k4_, k5_, k6_, p1_, p2_, s1_, s2_, s3_, s4_}; }

   private:
    float k1_;
    float k2_;
    float k3_;
    float k4_;
    float k5_;
    float k6_;
    float p1_;
    float p2_;
    float s1_;
    float s2_;
    float s3_;
    float s4_;
};

}  // namespace aisdk::base