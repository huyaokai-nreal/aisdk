#include "hand_rotation_v2.h"

#include <Eigen/src/Geometry/AngleAxis.h>

#include "aisdk/base/log.h"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {

std::map<int, int> xr_parent_index = {{1, 0},   {5, 0},   {9, 0},  {13, 0},  {22, 0},  {23, 0},  {24, 0},  {25, 0},
                                      {2, 1},   {6, 5},   {10, 9}, {14, 13}, {17, 22}, {3, 2},   {7, 6},   {11, 10},
                                      {15, 14}, {18, 17}, {4, 3},  {8, 7},   {12, 11}, {16, 15}, {19, 18}, {20, 19}};
const std::vector<int> xr_lev1_index = {1, 5, 9, 13, 22, 23, 24, 25};
const std::vector<int> xr_lev2_index = {2, 6, 10, 14, 17};
const std::vector<int> xr_lev3_index = {3, 7, 11, 15, 18};
const std::vector<int> xr_lev4_index = {4, 8, 12, 16, 19};

Eigen::Matrix3f get_root_rotation_xr_v1(const std::vector<Vec3f_t>& joints, bool left_hand) {
    Vec3f_t v0_y = joints[9] - joints[0];
    Vec3f_t vec_1, vec_2;
    vec_1 = {0, 0, -1};
    vec_2 = {v0_y[0], v0_y[1], v0_y[2]};
    Eigen::Matrix3f R0_y = Eigen::Quaternionf::FromTwoVectors(vec_1, vec_2).toRotationMatrix();

    Vec3f_t v0_z_temp = v0_y.cross(joints[5] - joints[0]);

    if (left_hand) {
        v0_z_temp *= -1;
    }
    Vec3f_t v0_z = v0_z_temp.normalized();
    vec_1 = {0, 1, 0};
    vec_1 = R0_y * vec_1;
    vec_2 = {v0_z[0], v0_z[1], v0_z[2]};
    Eigen::Matrix3f R0_z = Eigen::Quaternionf::FromTwoVectors(vec_1, vec_2).toRotationMatrix();
    Eigen::Matrix3f R0 = R0_z * R0_y;
    return R0;
}

bool compute_xr_joint_rotation_v1(const std::vector<Vec3f_t>& joint, bool left_hand,
                                  std::vector<Eigen::Matrix3f>& rotations_world) {
    rotations_world.clear();
    rotations_world.resize(26);

    Eigen::Matrix3f R0 = get_root_rotation_xr_v1(joint, left_hand);
    rotations_world[0] = R0;

    Vec3f_t vec_1, vec_2;
    for (auto lev1_i : xr_lev1_index) {
        int parent = xr_parent_index[lev1_i];
        auto vec1 = joint[9] - joint[parent];
        auto vec2 = joint[lev1_i] - joint[parent];
        vec_1 = {vec1[0], vec1[1], vec1[2]};
        vec_2 = {vec2[0], vec2[1], vec2[2]};
        Eigen::Matrix3f Ri = Eigen::Quaternionf::FromTwoVectors(vec_1, vec_2).toRotationMatrix();
        rotations_world[lev1_i] = Ri * R0;
    }
    // hand center
    rotations_world[21] = rotations_world[9];

    float angle = -60;

    if (left_hand) {
        angle *= -1;
    }

    auto vec_ = joint[1] - joint[0];
    vec_1 = {vec_[0], vec_[1], vec_[2]};
    vec_1.normalize();
    Eigen::Matrix3f R_thumb = Eigen::AngleAxisf((angle / 180) * 3.14, vec_1).toRotationMatrix();
    rotations_world[1] = R_thumb * rotations_world[1];

    for (int i = 0; i < 5; i++) {
        int lev1_parent = xr_parent_index[xr_lev1_index[i]];
        int lev2_parent = xr_parent_index[xr_lev2_index[i]];
        auto vec1 = joint[xr_lev1_index[i]] - joint[lev1_parent];
        auto vec2 = joint[xr_lev2_index[i]] - joint[lev2_parent];
        vec_1 = {vec1[0], vec1[1], vec1[2]};
        vec_2 = {vec2[0], vec2[1], vec2[2]};
        Eigen::Matrix3f Ri = Eigen::Quaternionf::FromTwoVectors(vec_1, vec_2).toRotationMatrix();
        rotations_world[xr_lev1_index[i]] = Ri * rotations_world[xr_lev1_index[i]];
    }

    for (int i = 0; i < 5; i++) {
        int lev2_parent = xr_parent_index[xr_lev2_index[i]];
        int lev3_parent = xr_parent_index[xr_lev3_index[i]];
        auto vec1 = joint[xr_lev2_index[i]] - joint[lev2_parent];
        auto vec2 = joint[xr_lev3_index[i]] - joint[lev3_parent];
        vec_1 = {vec1[0], vec1[1], vec1[2]};
        vec_2 = {vec2[0], vec2[1], vec2[2]};
        Eigen::Matrix3f Ri = Eigen::Quaternionf::FromTwoVectors(vec_1, vec_2).toRotationMatrix();
        rotations_world[xr_lev2_index[i]] = Ri * rotations_world[lev2_parent];
    }

    for (int i = 0; i < 5; i++) {
        int lev3_parent = xr_parent_index[xr_lev3_index[i]];
        int lev4_parent = xr_parent_index[xr_lev4_index[i]];
        auto vec1 = joint[xr_lev3_index[i]] - joint[lev3_parent];
        auto vec2 = joint[xr_lev4_index[i]] - joint[lev4_parent];
        vec_1 = {vec1[0], vec1[1], vec1[2]};
        vec_2 = {vec2[0], vec2[1], vec2[2]};
        Eigen::Matrix3f Ri = Eigen::Quaternionf::FromTwoVectors(vec_1, vec_2).toRotationMatrix();
        rotations_world[xr_lev3_index[i]] = Ri * rotations_world[lev3_parent];
        rotations_world[xr_lev4_index[i]] = rotations_world[xr_lev3_index[i]];
    }

    int lev4_i = 19;
    int lev5_i = 20;
    int lev4_parent = xr_parent_index[lev4_i];
    int lev5_parent = xr_parent_index[lev5_i];

    auto vec1 = joint[lev4_i] - joint[lev4_parent];
    auto vec2 = joint[lev5_i] - joint[lev5_parent];
    vec_1 = {vec1[0], vec1[1], vec1[2]};
    vec_2 = {vec2[0], vec2[1], vec2[2]};
    Eigen::Matrix3f Ri = Eigen::Quaternionf::FromTwoVectors(vec_1, vec_2).toRotationMatrix();

    rotations_world[lev4_i] = Ri * rotations_world[lev4_parent];
    rotations_world[lev5_i] = rotations_world[lev4_i];

    return true;
}

}  // namespace aisdk::algorithm