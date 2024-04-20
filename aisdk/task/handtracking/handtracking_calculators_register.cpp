#include "handtracking_calculators_register.h"

#include "aisdk/algorithm/calculator/block_hard_rules_calculator.cpp"
#include "aisdk/algorithm/calculator/convert_to_world_calculator.cpp"
#include "aisdk/algorithm/calculator/detect_box_smoothing_calculator.cpp"
#include "aisdk/algorithm/calculator/gesture_recognition_calculator.cpp"
#include "aisdk/algorithm/calculator/hand_detection_calculator.cpp"
#include "aisdk/algorithm/calculator/hand_detection_track_calculator.cpp"
#include "aisdk/algorithm/calculator/hand_landmark_calculator.cpp"
#include "aisdk/algorithm/calculator/hand_lift_calculator.cpp"
#include "aisdk/algorithm/calculator/kalman_filter_correction_calculator.cpp"
#include "aisdk/algorithm/calculator/landmark_filter_calculator.cpp"
#include "aisdk/algorithm/calculator/standardize_keypoints_calculator.cpp"

namespace aisdk::task {

// 这里是要规避全局类不构造的问题, 以后找到原因解决
void TriggerGloalGraphCalculatorsConstructForHandTracking() {
    using namespace algorithm;
    REGISTER_CALCULATOR(BlockHardRulesCalculator);          // ok!
    REGISTER_CALCULATOR(ConvertToWorldCalculator);          // ok!
    REGISTER_CALCULATOR(DetectBoxSmoothingCalculator);      // ok!
    REGISTER_CALCULATOR(HandDetectionCalculator);           // ok!
    REGISTER_CALCULATOR(GestureRecognitionCalculator);      // ok!
    REGISTER_CALCULATOR(HandLandmarkCalculator);            // ok!
    REGISTER_CALCULATOR(KalmanFilterCorrectionCalculator);  // ok!
    REGISTER_CALCULATOR(HandLiftCalculator);                // ok!
    REGISTER_CALCULATOR(StandardizeKeypointsCalculator);    // ok！
    REGISTER_CALCULATOR(LandmarkFilterCalculator);          // ok！
    REGISTER_CALCULATOR(HandDetTrackCalculator);            // ok！
}
// 这里是要规避全局类不构造的问题, 以后找到原因解决
void TriggerGloalGraphCalculatorsConstruct() {
    static std::once_flag oc;
    std::call_once(oc, [&]() { aisdk::task::TriggerGloalGraphCalculatorsConstructForHandTracking(); });
}

}  // namespace aisdk::task
