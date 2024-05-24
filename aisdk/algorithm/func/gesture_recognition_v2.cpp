#include "gesture_recognition_v2.h"

#include <functional>
#include <map>

#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/base/log.h"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {
std::map<HandGesture, std::function<bool(const HandFeature &, const HandRawFeature &)>> GestureFunctionMap = {
    {HandGesture::Click, GestureMatchRule::Click},       {HandGesture::Grab, GestureMatchRule::Grab},
    {HandGesture::OpenHand, GestureMatchRule::OpenHand}, {HandGesture::Pinch, GestureMatchRule::Pinch},
    {HandGesture::Victory, GestureMatchRule::Victory},   {HandGesture::Call, GestureMatchRule::Call},
    {HandGesture::Home, GestureMatchRule::Home},         {HandGesture::ThumbUp, GestureMatchRule::ThumbUp}};

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
    } else if (60 <= hand_angle && 60 <= 120) {
        cur_hand_feature->set_orientation_feature(HandOrientation::VERTICAL);
    } else {
        cur_hand_feature->set_orientation_feature(HandOrientation::OTHER);
    }
}

void HandFeatureUpdator::update_curl_feature(const std::vector<Eigen::Vector3f> &angles) {
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
}

void HandFeatureUpdator::update_opposition_feature(const std::vector<float> &distances, bool relax_th_flag) {
    auto feature_list = cur_hand_feature->opposition_features();
    float opposition_closed_th = this->opposition_closed_th;
    float opposition_th_width = this->opposition_th_width;
    if (relax_th_flag) {
        opposition_closed_th = this->opposition_relax_closed_th;
        opposition_th_width = this->opposition_relax_th_width;
    }
    AISDK_LOG_TRACE("HandTracking: pinch distance is {}", distances[0]);
    for (int id = 0; id < feature_list.size() - 1; id++) {
        float distance = distances[id];
        feature_list[id + 1] = update_fingure_state(distance, opposition_closed_th, this->opposition_open_th,
                                                    opposition_th_width, feature_list[id + 1]);
    }
    cur_hand_feature->set_opposition_features(feature_list);
}

Eigen::Vector3f GetPlaneNormalVectorFrom3Points(const Eigen::Vector3f &pt_o, const Eigen::Vector3f &pt_a,
                                                const Eigen::Vector3f &pt_b) {
    Eigen::Vector3f oa = pt_a - pt_o;
    Eigen::Vector3f ob = pt_b - pt_o;
    Eigen::Vector3f N = oa.cross(ob);
    return N.normalized();
}

float from_two_vectors(const Eigen::Vector3f &src_vec, const Eigen::Vector3f &dst_vec) {
    Eigen::Vector3f norm_src_vec = src_vec.normalized();
    Eigen::Vector3f norm_dst_vec = dst_vec.normalized();
    float angle = norm_src_vec.dot(norm_dst_vec);
    return acos(angle) / M_PI * 180;
}
bool isInsideTriangle(const Eigen::Vector2f &pt, const Eigen::Vector2f &v1, const Eigen::Vector2f &v2,
                      const Eigen::Vector2f &v3, float scale = 1.0) {
    // rescale the triangle
    Eigen::Vector2f mean_v = (v1 + v2 + v3) / 3.0F;
    Eigen::Vector2f new_v1 = (v1 - mean_v) * scale + mean_v;
    Eigen::Vector2f new_v2 = (v2 - mean_v) * scale + mean_v;
    Eigen::Vector2f new_v3 = (v3 - mean_v) * scale + mean_v;

    // 计算向量
    Eigen::Vector2f v2_v1 = new_v2 - new_v1;
    Eigen::Vector2f v3_v1 = new_v3 - new_v1;
    Eigen::Vector2f pt_v1 = pt - new_v1;

    // 计算点积和行列式
    double dot00 = v2_v1.dot(v2_v1);
    double dot01 = v2_v1.dot(v3_v1);
    double dot02 = v2_v1.dot(pt_v1);
    double dot11 = v3_v1.dot(v3_v1);
    double dot12 = v3_v1.dot(pt_v1);

    // 计算重心坐标
    double invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
    double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    // 检查点是否在三角形内
    return (u >= 0) && (v >= 0) && (u + v < 1);
}
bool GestureRecognitionV2::is_face_to_head(const std::vector<std::vector<Eigen::Vector3f>> &keypoints3d,
                                           bool is_left_hand) {
    // whether the hand if face to ego face
    const auto &root_pt_3d = keypoints3d[0][0];
    const auto &index_pt_3d = keypoints3d[1][1];
    const auto &ring_pt_3d = keypoints3d[3][1];
    Eigen::Vector3f palm_norm_vec = Eigen::Vector3f::Identity();
    if (is_left_hand) {
        palm_norm_vec = GetPlaneNormalVectorFrom3Points(root_pt_3d, ring_pt_3d, index_pt_3d);
    } else {
        palm_norm_vec = GetPlaneNormalVectorFrom3Points(root_pt_3d, index_pt_3d, ring_pt_3d);
    }
    float angle_to_face = from_two_vectors(palm_norm_vec, {0, 0, -1});
    return angle_to_face < 60;
}
bool GestureRecognitionV2::is_ok_pinch(const std::vector<std::vector<Eigen::Vector3f>> &keypoints3d) {
    Eigen::Vector3f thumb_direction = (keypoints3d[0][4] - keypoints3d[0][3]).normalized();
    Eigen::Vector3f index_direction = (keypoints3d[1][4] - keypoints3d[1][3]).normalized();
    float pinch_figure_angle = from_two_vectors(thumb_direction, index_direction);
    AISDK_LOG_TRACE("HandTracking: pinch angle: {}", pinch_figure_angle);
    return pinch_figure_angle > 80;
}
bool GestureRecognitionV2::is_pinch_masked(const std::vector<Vec2f_t> &keypoints2d, bool is_to_face) {
    Eigen::Vector2f thumb_mid_2d{keypoints2d[1][0], keypoints2d[1][1]};
    Eigen::Vector2f thumb_point_2d{keypoints2d[4][0], keypoints2d[4][1]};
    Eigen::Vector2f index_mid_pt_2d{keypoints2d[6][0], keypoints2d[6][1]};
    Eigen::Vector2f pinky_mid_2d{keypoints2d[17][0], keypoints2d[17][1]};
    Eigen::Vector2f index_point_2d{keypoints2d[8][0], keypoints2d[8][1]};
    bool is_thumb_point_masked = isInsideTriangle(thumb_point_2d, thumb_mid_2d, index_mid_pt_2d, pinky_mid_2d, 1.1);
    bool is_index_point_masked = isInsideTriangle(index_point_2d, thumb_mid_2d, index_mid_pt_2d, pinky_mid_2d, 1.1);
    bool is_pinch_masked =
        is_thumb_point_masked && is_index_point_masked && !is_to_face && last_gesture_ == HandGesture::Pinch;
    AISDK_LOG_TRACE("HandTracking: pinch mask flag is {}", is_pinch_masked);
    return is_pinch_masked;
}
std::pair<HandRawFeature, HandFeature> GestureRecognitionV2::extract_hand_feature(
    const std::vector<std::vector<Eigen::Vector3f>> &keypoints3d, const std::vector<Vec2f_t> &keypoints2d,
    bool is_left_hand) {
    auto fingure_angles = calculate_fingure_angles(keypoints3d);
    auto abduction_angles = calculate_abduction_angles(keypoints3d);
    auto opposition_distances = calculate_opposition_distances(keypoints3d);
    auto hand_angle = vector3d_angle(keypoints3d[2][4] - keypoints3d[2][1], Eigen::Vector3f(1, 0, 0));
    HandRawFeature raw_features;
    raw_features.fingure_angles = fingure_angles;
    raw_features.abduction_angles = abduction_angles;
    raw_features.opposition_distances = opposition_distances;
    raw_features.hand_angle = hand_angle;
    raw_features.is_thumb_up = (keypoints3d[0][4](1) - keypoints3d[0][2](1)) > 0.f;
    bool is_to_face = is_face_to_head(keypoints3d, is_left_hand);
    bool ok_pinch = is_ok_pinch(keypoints3d);
    bool pinch_masked = is_pinch_masked(keypoints2d, is_to_face);
    // whether pinch point is masked
    return {raw_features, feature_updator->update(fingure_angles, abduction_angles, opposition_distances, hand_angle,
                                                  is_to_face || ok_pinch, pinch_masked)};
}
std::pair<HandGesture, HandRawFeature> GestureRecognitionV2::predict_with_keypoints3d(
    const std::vector<Eigen::Vector3f> &keypoints3d, const std::vector<Vec2f_t> &keypoints2d, bool is_left_hand) {
    std::vector<std::vector<Eigen::Vector3f>> points(5, std::vector<Eigen::Vector3f>(5));
    float hand_length = (keypoints3d[9] - keypoints3d[0]).norm();
    const auto &root_kpt = keypoints3d[0];
    for (int i = 0; i < 5; i++) {
        points[i][0] = keypoints3d[0] - root_kpt;
        for (int j = 0; j < 4; j++) {
            points[i][j + 1] = (keypoints3d[i * 4 + j + 1] - root_kpt) * std_hand_length_ / hand_length;
        }
    }

    auto [raw_features, feature] = extract_hand_feature(points, keypoints2d, is_left_hand);
    raw_features.feature = feature;

    for (int i = static_cast<int>(HandGesture::Invalid); i < static_cast<int>(HandGesture::MaxNum); i++) {
        auto cur_gesture = static_cast<HandGesture>(i);
        if (GestureFunctionMap.count(cur_gesture) > 0) {
            if (GestureFunctionMap[cur_gesture](feature, raw_features)) {
                last_gesture_ = cur_gesture;
                return {cur_gesture, raw_features};
            }
        }
    }
    last_gesture_ = HandGesture::Invalid;

    return {HandGesture::Invalid, raw_features};
}
}  // namespace aisdk::algorithm
