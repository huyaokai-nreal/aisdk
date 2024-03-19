#include "NR_Gesture_v2.h"

#include <functional>
#include <iostream>

namespace aisdk::algorithm {

std::map<std::string, std::function<bool(const HandFeature &, const HandRawFeature &)>> gestureFunctionMap = {
    {"Click", GestureMatchRule::Click},       {"Grab", GestureMatchRule::Grab},
    {"OpenHand", GestureMatchRule::OpenHand}, {"Pinch", GestureMatchRule::Pinch},
    {"Victory", GestureMatchRule::Victory},   {"Call", GestureMatchRule::Call},
    {"Home", GestureMatchRule::Home},         {"ThumbUp", GestureMatchRule::ThumbUp}};

float vector3d_angle(const Eigen::Vector3f &x, const Eigen::Vector3f &y) {
    float module_x = x.norm();
    float module_y = y.norm();
    float dot_value = x.dot(y);
    float cos_theta = dot_value / (module_x * module_y);
    float angle_radian = std::acos(cos_theta);
    float angle_value = angle_radian * 180.0f / M_PI;  // M_PI is defined in <cmath>
    return angle_value;
}

std::vector<Eigen::Vector3f> calculate_fingure_angles(const std::vector<std::vector<Eigen::Vector3f>> &points) {
    assert(points.size() == 5 && "There should be 5 fingers");
    for (const auto &finger : points) {
        assert(finger.size() == 5 && "Each finger should have 5 points");
    }

    std::vector<Eigen::Vector3f> angles(5);

    for (size_t i = 0; i < 5; ++i) {
        angles[i][0] = vector3d_angle(points[i][1] - points[i][0], points[i][2] - points[i][1]);
        angles[i][1] = vector3d_angle(points[i][2] - points[i][1], points[i][3] - points[i][2]);
        angles[i][2] = vector3d_angle(points[i][3] - points[i][2], points[i][4] - points[i][3]);
    }

    return angles;
}

std::vector<float> calculate_opposition_distances(const std::vector<std::vector<Eigen::Vector3f>> &points) {
    std::vector<float> distances(4);
    for (size_t i = 1; i < 5; ++i) {
        distances[i - 1] = (points[i].back() - points[0].back()).norm();
    }
    return distances;
}

std::vector<float> calculate_abduction_angles(const std::vector<std::vector<Eigen::Vector3f>> &points) {
    std::vector<float> angles(4);
    for (size_t i = 0; i < 4; ++i) {
        angles[i] = vector3d_angle(points[i][2] - points[i][0], points[i + 1][2] - points[i + 1][0]);
    }
    return angles;
}

FingureState HandFeatureUpdator::update_fingure_state(float value, float close_th, float open_th, float width,
                                                      FingureState state) {
    if (value <= close_th - width / 2) {
        state = FingureState::CLOSED;
    } else if (((close_th + width / 2) < value) && (value < (open_th - width / 2))) {
        state = FingureState::NEUTRAL;
    } else if (value > open_th + width / 2) {
        state = FingureState::OPEN;
    }
    return state;
}

void HandFeatureUpdator::update_hand_orientation(float hand_angle) {
    if ((0 <= hand_angle && hand_angle <= 30) || (150 <= hand_angle && hand_angle <= 180)) {
        cur_hand_feature->set_orientation_feature(HandOrientation::HORIZONTAL);
    } else if (60 <= hand_angle <= 120) {
        cur_hand_feature->set_orientation_feature(HandOrientation::VERTICAL);
    } else {
        cur_hand_feature->set_orientation_feature(HandOrientation::OTHER);
    }
    return;
}

void HandFeatureUpdator::update_curl_feature(const std::vector<Eigen::Vector3f> &angles) {
    int angle_size = angles.size();
    int per_angle_size = angles[0].size();
    auto feature_list = cur_hand_feature->curl_features();
    FingureState thumb_feature = feature_list[0];
    float thumb_angle = 180 - angles[0][per_angle_size - 1];

    feature_list[0] = update_fingure_state(thumb_angle, this->curl_thumb_closed_th, this->curl_thumb_open_th,
                                           this->curl_th_width, thumb_feature);

    for (int id = 0; id < feature_list.size() - 1; id++) {
        float angle = 180 - angles[id + 1](1) - angles[id + 1](2);
        feature_list[id + 1] = update_fingure_state(angle, this->curl_other_closed_th, this->curl_other_open_th,
                                                    this->curl_th_width, feature_list[id + 1]);
    }
    cur_hand_feature->set_curl_features(feature_list);
    return;
}

void HandFeatureUpdator::update_flexion_feature(const std::vector<Eigen::Vector3f> &angles) {
    auto feature_list = cur_hand_feature->curl_features();
    FingureState thumb_feature = feature_list[0];
    float thumb_angle = 180 - angles[0][1];

    feature_list[0] = update_fingure_state(thumb_angle, this->flexion_thumb_closed_th, this->flexion_thumb_open_th,
                                           this->flexion_th_width, thumb_feature);

    for (int id = 0; id < feature_list.size() - 1; id++) {
        float angle = 180 - angles[id + 1][0];
        feature_list[id + 1] = update_fingure_state(angle, this->flexion_other_closed_th, this->flexion_other_open_th,
                                                    this->flexion_th_width, feature_list[id + 1]);
    }
    cur_hand_feature->set_flexion_features(feature_list);
    return;
}

void HandFeatureUpdator::update_abduction_feature(const std::vector<float> &angles) {
    auto feature_list = cur_hand_feature->abduction_features();
    FingureState thumb_feature = feature_list[0];
    float thumb_angle = angles[0];
    feature_list[0] = update_fingure_state(thumb_angle, this->abduction_thumb_closed_th, this->abduction_thumb_open_th,
                                           this->abduction_th_width, thumb_feature);
    for (int id = 0; id < feature_list.size() - 2; id++) {
        float angle = angles[id + 1];
        feature_list[id + 1] =
            update_fingure_state(angle, this->abduction_other_closed_th, this->abduction_other_open_th,
                                 this->abduction_th_width, feature_list[id + 1]);
    }
    cur_hand_feature->set_abduction_features(feature_list);
    return;
}

void HandFeatureUpdator::update_opposition_feature(const std::vector<float> &distances) {
    auto feature_list = cur_hand_feature->opposition_features();
    for (int id = 0; id < feature_list.size() - 1; id++) {
        float distance = distances[id];
        feature_list[id + 1] = update_fingure_state(distance, this->opposition_closed_th, this->opposition_open_th,
                                                    this->opposition_th_width, feature_list[id + 1]);
    }
    cur_hand_feature->set_opposition_features(feature_list);
    return;
}

std::pair<HandRawFeature, HandFeature> GestureRecognitionV2::extract_hand_feature(
    const std::vector<std::vector<Eigen::Vector3f>> &keypoints3d) {
    auto fingure_angles = calculate_fingure_angles(keypoints3d);
    auto abduction_angles = calculate_abduction_angles(keypoints3d);
    auto opposition_distances = calculate_opposition_distances(keypoints3d);
    auto hand_angle = vector3d_angle(keypoints3d[2][4] - keypoints3d[2][1], Eigen::Vector3f(1, 0, 0));

    HandRawFeature raw_features;
    raw_features.fingure_angles = fingure_angles;
    raw_features.abduction_angles = abduction_angles;
    raw_features.opposition_distances = opposition_distances;
    raw_features.hand_angle = hand_angle;
    raw_features.is_thumb_up = (keypoints3d[0][4](1) - keypoints3d[0][2](1)) < 0.f;

    // AISDK_LOG_TRACE("keypoints3d[0][4]: {}, {}, {}", keypoints3d[0][4](0), keypoints3d[0][4](1),
    // keypoints3d[0][4](2)); AISDK_LOG_TRACE("keypoints3d[0][2]: {}, {}, {}", keypoints3d[0][2](0),
    // keypoints3d[0][2](1), keypoints3d[0][2](2));

    return {raw_features, feature_updator->update(fingure_angles, abduction_angles, opposition_distances, hand_angle)};
}
std::pair<std::string, HandRawFeature> GestureRecognitionV2::predict_with_keypoints3d(
    const std::vector<Eigen::Vector3f> &keypoints3d) {
    std::vector<std::vector<Eigen::Vector3f>> points(5, std::vector<Eigen::Vector3f>(5));
    for (int i = 0; i < 5; i++) {
        points[i][0] = keypoints3d[0];
        for (int j = 0; j < 4; j++) {
            points[i][j + 1] = keypoints3d[i * 4 + j + 1];
        }
    }

    auto [raw_features, feature] = extract_hand_feature(points);
    raw_features.feature = feature;

    for (const auto &gesture : gesture_list) {
        if (gestureFunctionMap[gesture](feature, raw_features)) {
            return {gesture, raw_features};
        }
    }

    return {"Invalid", raw_features};
}
}  // namespace aisdk::algorithm
