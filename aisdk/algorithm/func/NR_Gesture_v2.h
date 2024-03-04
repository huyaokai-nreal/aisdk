#pragma once

#include <Eigen/Dense>
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

namespace aisdk::algorithm {

enum class FingureState { UNKNOWN = 0, OPEN, NEUTRAL, CLOSED };

enum class HandOrientation { UNKNOWN = 0, HORIZONTAL, VERTICAL, OTHER };

struct FingureFeature {
    // 手指指尖相对于手指的角度
    FingureState curl = FingureState::UNKNOWN;

    // 手指与手掌之间的角度
    FingureState flexion = FingureState::UNKNOWN;

    // 两指之间的开合角度
    FingureState abduction = FingureState::UNKNOWN;

    // 拇指与其他四指的距离
    FingureState opposition = FingureState::UNKNOWN;
};

class HandFeature {
   private:
    HandOrientation orientation = HandOrientation::UNKNOWN;
    FingureFeature thumb, index, middle, ring, pinky;

   public:
    HandFeature() : thumb(), index(), middle(), ring(), pinky() {}

    void set_curl_features(const std::array<FingureState, 5> &features) {
        thumb.curl = features[0];
        index.curl = features[1];
        middle.curl = features[2];
        ring.curl = features[3];
        pinky.curl = features[4];
    }

    void set_flexion_features(const std::array<FingureState, 5> &features) {
        thumb.flexion = features[0];
        index.flexion = features[1];
        middle.flexion = features[2];
        ring.flexion = features[3];
        pinky.flexion = features[4];
    }

    void set_abduction_features(const std::array<FingureState, 5> &features) {
        thumb.abduction = features[0];
        index.abduction = features[1];
        middle.abduction = features[2];
        ring.abduction = features[3];
        pinky.abduction = features[4];
    }

    void set_opposition_features(const std::array<FingureState, 5> &features) {
        thumb.opposition = features[0];
        index.opposition = features[1];
        middle.opposition = features[2];
        ring.opposition = features[3];
        pinky.opposition = features[4];
    }

    void set_orientation_feature(const HandOrientation feature) { orientation = feature; }

    std::array<FingureState, 5> curl_features() const {
        return {thumb.curl, index.curl, middle.curl, ring.curl, pinky.curl};
    }

    std::array<FingureState, 5> flexion_features() const {
        return {thumb.flexion, index.flexion, middle.flexion, ring.flexion, pinky.flexion};
    }

    std::array<FingureState, 5> abduction_features() const {
        return {thumb.abduction, index.abduction, middle.abduction, ring.abduction, pinky.abduction};
    }

    std::array<FingureState, 5> opposition_features() const {
        return {thumb.opposition, index.opposition, middle.opposition, ring.opposition, pinky.opposition};
    }

    HandOrientation orientation_feature() const { return orientation; }
};

struct HandRawFeature {
    std::vector<Eigen::Vector3f> fingure_angles;
    std::vector<float> abduction_angles;
    std::vector<float> opposition_distances;
    float hand_angle;
    bool is_thumb_up;
    HandFeature feature;
};

class HandFeatureUpdator {
   public:
    HandFeatureUpdator() : cur_hand_feature(std::make_unique<HandFeature>()) {}
    void reset() {
        cur_hand_feature = std::make_unique<HandFeature>();
        return;
    }

    HandFeature update(const std::vector<Eigen::Vector3f> &fingure_angles, std::vector<float> &abduction_angles,
                       const std::vector<float> &distances, float hand_angle) {
        update_curl_feature(fingure_angles);
        update_flexion_feature(fingure_angles);
        update_abduction_feature(abduction_angles);
        update_opposition_feature(distances);
        update_hand_orientation(hand_angle);

        return *cur_hand_feature;
    }

    FingureState update_fingure_state(float value, float close_th, float open_th, float width, FingureState state);

    void update_curl_feature(const std::vector<Eigen::Vector3f> &angles);
    void update_flexion_feature(const std::vector<Eigen::Vector3f> &angles);
    void update_abduction_feature(const std::vector<float> &angles);
    void update_opposition_feature(const std::vector<float> &distances);
    void update_hand_orientation(float hand_angle);

   private:
    float curl_th_width = 10;
    float curl_thumb_open_th = 140;
    float curl_thumb_closed_th = 130;
    float curl_other_open_th = 148;
    float curl_other_closed_th = 70;

    float flexion_th_width = 8;
    float flexion_thumb_open_th = 150;
    float flexion_thumb_closed_th = 120;
    float flexion_other_open_th = 145;
    float flexion_other_closed_th = 110;

    float abduction_th_width = 2;
    float abduction_thumb_open_th = 42;
    float abduction_thumb_closed_th = 20;
    float abduction_other_open_th = 12;
    float abduction_other_closed_th = 10;

    // 0.04 0.025

    float opposition_closed_th = 0.0325;  // 3.25cm
    float opposition_open_th = 0.08;      // 8cm
    float opposition_th_width = 0.015;    // 1.5cm

    std::unique_ptr<HandFeature> cur_hand_feature;
};

class GestureMatchRule {
   public:
    static bool Click(const HandFeature &hand_feature, const HandRawFeature &raw_feature) {
        auto [thumb_curl, index_curl, middle_curl, ring_curl, pinky_curl] = hand_feature.curl_features();
        auto [thumb_opposition, index_opposition, middle_opposition, ring_opposition, pinky_opposition] =
            hand_feature.opposition_features();
        return index_curl == FingureState::OPEN && middle_curl == FingureState::CLOSED &&
               ring_curl == FingureState::CLOSED && pinky_curl == FingureState::CLOSED &&
               index_opposition != FingureState::CLOSED;
    }

    static bool Grab(const HandFeature &hand_feature, const HandRawFeature &raw_feature) {
        auto [thumb_curl, index_curl, middle_curl, ring_curl, pinky_curl] = hand_feature.curl_features();
        return index_curl == FingureState::CLOSED && middle_curl == FingureState::CLOSED &&
               ring_curl == FingureState::CLOSED && pinky_curl == FingureState::CLOSED &&
               thumb_curl != FingureState::OPEN;
    }

    static bool OpenHand(const HandFeature &hand_feature, const HandRawFeature &raw_feature) {
        auto [thumb_curl, index_curl, middle_curl, ring_curl, pinky_curl] = hand_feature.curl_features();
        auto [thumb_flexion, index_flexion, middle_flexion, ring_flexion, pinky_flexion] =
            hand_feature.flexion_features();

        return thumb_curl == FingureState::OPEN && index_curl == FingureState::OPEN &&
               middle_curl == FingureState::OPEN && ring_curl == FingureState::OPEN &&
               pinky_curl == FingureState::OPEN && index_flexion == FingureState::OPEN &&
               middle_flexion == FingureState::OPEN && ring_flexion == FingureState::OPEN &&
               pinky_flexion == FingureState::OPEN;
    }

    static bool Pinch(const HandFeature &hand_feature, const HandRawFeature &raw_feature) {
        auto [_, index_opposition, __, ___, ____] = hand_feature.opposition_features();
        return index_opposition == FingureState::CLOSED;
    }

    static bool Victory(const HandFeature &hand_feature, const HandRawFeature &raw_feature) {
        auto [_, index_curl, middle_curl, ring_curl, pinky_curl] = hand_feature.curl_features();
        auto [__, index_abduction, ___, ____, _____] = hand_feature.abduction_features();

        return index_curl == FingureState::OPEN && middle_curl == FingureState::OPEN &&
               ring_curl == FingureState::CLOSED && pinky_curl != FingureState::OPEN &&
               index_abduction == FingureState::OPEN;
    }

    static bool Call(const HandFeature &hand_feature, const HandRawFeature &raw_feature) {
        auto [thumb_curl, index_curl, middle_curl, ring_curl, pinky_curl] = hand_feature.curl_features();
        auto [thumb_flexion, index_flexion, middle_flexion, ring_flexion, pinky_flexion] =
            hand_feature.flexion_features();

        return thumb_curl == FingureState::OPEN &&
               (index_curl == FingureState::CLOSED || index_flexion == FingureState::CLOSED) &&
               (middle_curl == FingureState::CLOSED || middle_flexion == FingureState::CLOSED) &&
               (ring_curl == FingureState::CLOSED || ring_flexion == FingureState::CLOSED) &&
               pinky_curl != FingureState::CLOSED;
    }

    static bool Home(const HandFeature &hand_feature, const HandRawFeature &raw_feature) {
        auto [thumb_curl, index_curl, middle_curl, ring_curl, pinky_curl] = hand_feature.curl_features();
        auto [thumb_flexion, _, __, ___, ____] = hand_feature.flexion_features();

        return (thumb_curl == FingureState::CLOSED || thumb_flexion == FingureState::CLOSED) &&
               index_curl == FingureState::OPEN && middle_curl == FingureState::OPEN &&
               ring_curl == FingureState::OPEN && pinky_curl == FingureState::OPEN &&
               hand_feature.orientation_feature() == HandOrientation::HORIZONTAL;
    }

    static bool ThumbUp(const HandFeature &hand_feature, const HandRawFeature &raw_feature) {
        auto [thumb_curl, index_curl, middle_curl, ring_curl, pinky_curl] = hand_feature.curl_features();
        auto [thumb_flexion, _, __, ___, ____] = hand_feature.flexion_features();
        auto [thumb_abduction, _p, __p, ___p, ____p] = hand_feature.abduction_features();

        return index_curl == FingureState::CLOSED && middle_curl == FingureState::CLOSED &&
               ring_curl == FingureState::CLOSED && pinky_curl == FingureState::CLOSED &&
               thumb_abduction == FingureState::OPEN && thumb_curl != FingureState::CLOSED && (raw_feature.is_thumb_up);
    }
};

class GestureRecognitionV2 {
   public:
    GestureRecognitionV2()
        : feature_updator(std::make_unique<HandFeatureUpdator>()),
          gesture_list({"Click", "Grab", "Pinch", "ThumbUp", "OpenHand", "Victory", "Call", "Home"}) {}

    void reset() {
        feature_updator = std::make_unique<HandFeatureUpdator>();
        return;
    }

    std::pair<HandRawFeature, HandFeature> extract_hand_feature(
        const std::vector<std::vector<Eigen::Vector3f>> &keypoints3d);
    std::pair<std::string, HandRawFeature> predict_with_keypoints3d(const std::vector<Eigen::Vector3f> &keypoints3d);

   private:
    std::unique_ptr<HandFeatureUpdator> feature_updator;
    std::vector<std::string> gesture_list;
};

}  // namespace aisdk::algorithm
