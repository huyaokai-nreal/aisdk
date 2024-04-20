#include "hand_rotation.h"
namespace aisdk::algorithm {

std::map<int, int> parent_index = {{1, 0},   {5, 0},   {9, 0},   {13, 0},  {22, 0},  {2, 1},   {6, 5},
                                   {10, 9},  {14, 13}, {17, 22}, {3, 2},   {7, 6},   {11, 10}, {15, 14},
                                   {18, 17}, {4, 3},   {8, 7},   {12, 11}, {16, 15}, {19, 18}, {20, 19}};
const std::vector<int> lev1_index = {1, 5, 9, 13, 22};
const std::vector<int> lev2_index = {2, 6, 10, 14, 17};
const std::vector<int> lev3_index = {3, 7, 11, 15, 18};
const std::vector<int> lev4_index = {4, 8, 12, 16, 19};
std::vector<Vec3f_t> get_metacarpal_joints_v1(const std::vector<Vec3f_t>& joints) {
    const auto& root_joint = joints[0];
    auto little_vec = ((root_joint - joints[9]) + (root_joint - joints[17])).normalized();
    auto little_metacarpal = joints[17] + 0.6667 * (joints[0] - joints[17]).norm() * little_vec;

    auto ring_vec = ((root_joint - joints[9]) + (root_joint - joints[13])).normalized();
    auto ring_metacarpal = joints[13] + 0.6667 * (joints[0] - joints[13]).norm() * ring_vec;

    auto middle_vec = (root_joint - joints[9]).normalized();
    auto middle_metacarpal = joints[9] + 0.6667 * (joints[0] - joints[9]).norm() * middle_vec;

    auto index_vec = (2 * middle_vec - ring_vec).normalized();
    auto index_metacarpal = joints[5] + 0.6667 * (joints[0] - joints[5]).norm() * index_vec;
    return {index_metacarpal, middle_metacarpal, ring_metacarpal, little_metacarpal};
}
bool compute_joint_rotation(const std::vector<Vec3f_t>& joint, bool left_hand,
                            std::vector<Eigen::Matrix3d>& rotations_world,
                            std::vector<Eigen::Matrix3d>& rotations_local) {
    rotations_world.clear();
    rotations_local.clear();

    rotations_world.resize(23);
    rotations_local.resize(23);

    Eigen::Matrix3d gl_T_cv;
    gl_T_cv << 1, 0, 0, 0, -1, 0, 0, 0, -1;
    // Eigen::Isometry3d gl_T_cv = Eigen::Isometry3d::Identity();
    // gl_T_cv.rotate(gl_R_cv);
    auto cv_T_gl = gl_T_cv;

    Vec3f_t v0_y = joint[9] - joint[0];
    Eigen::Vector3d vec_1, vec_2;
    vec_1 = {0, 1, 0};
    vec_2 = {v0_y[0], v0_y[1], v0_y[2]};
    Eigen::Matrix3d R0_y = Eigen::Quaterniond::FromTwoVectors(vec_1, vec_2).toRotationMatrix();

    Vec3f_t v0_z_temp = v0_y.cross(joint[5] - joint[0]);

    if (left_hand) {
        v0_z_temp *= -1;
    }

    Vec3f_t v0_z = v0_z_temp.normalized();
    vec_1 = {0, 0, 1};
    vec_1 = R0_y * vec_1;
    vec_2 = {v0_z[0], v0_z[1], v0_z[2]};
    Eigen::Matrix3d R0_z = Eigen::Quaterniond::FromTwoVectors(vec_1, vec_2).toRotationMatrix();

    Eigen::Matrix3d R0 = R0_z * R0_y;
    rotations_world[0] = R0;
    rotations_local[0] = R0;

    // # lev1 1, 5, 9, 13, 17
    for (auto lev1_i : lev1_index) {
        int parent = parent_index[lev1_i];
        auto vec1 = joint[9] - joint[parent];
        auto vec2 = joint[lev1_i] - joint[parent];
        vec_1 = {vec1[0], vec1[1], vec1[2]};
        vec_2 = {vec2[0], vec2[1], vec2[2]};

        Eigen::Matrix3d Ri = Eigen::Quaterniond::FromTwoVectors(vec_1, vec_2).toRotationMatrix();
        rotations_world[lev1_i] = Ri * R0;
        rotations_local[lev1_i] = Ri;
    }

    rotations_world[21] = rotations_world[9];

    float angle = -60;

    if (left_hand) {
        angle *= -1;
    }

    auto vec_ = joint[1] - joint[0];
    vec_1 = {vec_[0], vec_[1], vec_[2]};
    vec_1.normalize();
    Eigen::Matrix3d R_thumb = Eigen::AngleAxisd((angle / 180) * 3.14, vec_1).toRotationMatrix();
    // R_thumb = axangle2mat(joints[:, 1] - joints[:, 0], (angle/180)*3.14,
    // is_normalized=False)
    rotations_world[1] = R_thumb * rotations_world[1];
    rotations_local[1] = R_thumb * rotations_local[1];

    // # lev2 2, 6, 10, 14, 18
    for (int i = 0; i < 5; i++) {
        int lev1_parent = parent_index[lev1_index[i]];
        int lev2_parent = parent_index[lev2_index[i]];
        auto vec1 = joint[lev1_index[i]] - joint[lev1_parent];
        auto vec2 = joint[lev2_index[i]] - joint[lev2_parent];
        vec_1 = {vec1[0], vec1[1], vec1[2]};
        vec_2 = {vec2[0], vec2[1], vec2[2]};
        Eigen::Matrix3d Ri = Eigen::Quaterniond::FromTwoVectors(vec_1, vec_2).toRotationMatrix();
        rotations_world[lev1_index[i]] = Ri * rotations_world[lev1_index[i]];
        rotations_local[lev1_index[i]] = Ri * rotations_local[lev1_index[i]];
    }

    // # lev3 3, 7, 11, 15, 19

    for (int i = 0; i < 5; i++) {
        int lev2_parent = parent_index[lev2_index[i]];
        int lev3_parent = parent_index[lev3_index[i]];
        auto vec1 = joint[lev2_index[i]] - joint[lev2_parent];
        auto vec2 = joint[lev3_index[i]] - joint[lev3_parent];
        vec_1 = {vec1[0], vec1[1], vec1[2]};
        vec_2 = {vec2[0], vec2[1], vec2[2]};
        Eigen::Matrix3d Ri = Eigen::Quaterniond::FromTwoVectors(vec_1, vec_2).toRotationMatrix();
        rotations_world[lev2_index[i]] = Ri * rotations_world[lev2_parent];
        rotations_local[lev2_index[i]] = Ri;
    }

    // # lev4 4, 8, 12, 16, 20

    for (int i = 0; i < 5; i++) {
        int lev3_parent = parent_index[lev3_index[i]];
        int lev4_parent = parent_index[lev4_index[i]];
        auto vec1 = joint[lev3_index[i]] - joint[lev3_parent];
        auto vec2 = joint[lev4_index[i]] - joint[lev4_parent];
        vec_1 = {vec1[0], vec1[1], vec1[2]};
        vec_2 = {vec2[0], vec2[1], vec2[2]};
        Eigen::Matrix3d Ri = Eigen::Quaterniond::FromTwoVectors(vec_1, vec_2).toRotationMatrix();
        rotations_world[lev3_index[i]] = Ri * rotations_world[lev3_parent];
        rotations_world[lev4_index[i]] = rotations_world[lev3_index[i]];
        rotations_local[lev3_index[i]] = Ri;
    }

    int lev4_i = 19;
    int lev5_i = 20;
    int lev4_parent = parent_index[lev4_i];
    int lev5_parent = parent_index[lev5_i];

    auto vec1 = joint[lev4_i] - joint[lev4_parent];
    auto vec2 = joint[lev5_i] - joint[lev5_parent];
    vec_1 = {vec1[0], vec1[1], vec1[2]};
    vec_2 = {vec2[0], vec2[1], vec2[2]};
    Eigen::Matrix3d Ri = Eigen::Quaterniond::FromTwoVectors(vec_1, vec_2).toRotationMatrix();

    rotations_world[lev4_i] = Ri * rotations_world[lev4_parent];
    rotations_world[lev5_i] = rotations_world[lev4_i];
    rotations_local[lev4_i] = Ri;

    return true;
}

}  // namespace aisdk::algorithm