#include "../calculator/block_hard_rules_calculator.cc"
#include "../calculator/check_headpose_calculator.cc"
#include "../calculator/compute_3dscore_calculator.cc"
#include "../calculator/convert_to_world_calculator.cc"
#include "../calculator/detect_box_smoothing_calculator.cc"
#include "../calculator/gesture_recognition_calculator.cc"
#include "../calculator/hand_detection_calculator.cc"
#include "../calculator/hand_detection_track_calculator.cc"
#include "../calculator/hand_landmark_calculator.cc"
#include "../calculator/hand_state_calculator.cc"
#include "../calculator/kalman_filter_correction_calculator.cc"
#include "../calculator/landmark_filter_calculator.cc"
#include "../calculator/lift_calculator.cc"
#include "../calculator/post_constraint_calculator.cc"
#include "../calculator/pre_constraint_calculator.cc"
#include "../calculator/standardize_keypoints_calculator.cc"

namespace mediapipe {
// 这里是要规避全局类不构造的问题, 以后找到原因解决
void TriggerGloalGraphCalculatorsConstructForHandTracking() {
    REGISTER_CALCULATOR(BlockHardRulesCalculator);          // ok!
    REGISTER_CALCULATOR(CheckHeadposeCalculator);           // not fully implemented
    REGISTER_CALCULATOR(Compute3DScoreCalculator);          // not fully implemented
    REGISTER_CALCULATOR(ConvertToWorldCalculator);          // ok!
    REGISTER_CALCULATOR(DetectBoxSmoothingCalculator);      // ok!
    REGISTER_CALCULATOR(HandDetectionCalculator);           // ok!
    REGISTER_CALCULATOR(GestureRecognitionCalculator);      // ok!
    REGISTER_CALCULATOR(HandLandmarkCalculator);            // ok!
    REGISTER_CALCULATOR(HandStateCalculator);               // ok！
    REGISTER_CALCULATOR(KalmanFilterCorrectionCalculator);  // ok!
    REGISTER_CALCULATOR(LiftCalculator);                    // ok!
    REGISTER_CALCULATOR(PostConstrainCalculator);           // ok!
    REGISTER_CALCULATOR(PreConstrainCalculator);            // ok!
    REGISTER_CALCULATOR(StandardizeKeypointsCalculator);    // ok！
    REGISTER_CALCULATOR(LandmarkFilterCalculator);          // ok！
    REGISTER_CALCULATOR(HandDetTrackCalculator);            // ok！
    return;
}

}  // namespace mediapipe
