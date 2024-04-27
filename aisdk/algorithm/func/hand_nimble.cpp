#include "hand_nimble.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <map>
#include <vector>

#include "aisdk/base/type.h"

namespace aisdk::algorithm {

static std::array<float, 75> NIMBLE_TEMPLATE_JOINTS{
    9.5550,   -22.8580, -102.1520, 35.5118,  -40.8386, -91.7473, 52.8191,  -66.0090, -61.6418, 59.9206,  -73.9186,
    -34.2990, 61.0230,  -84.2977,  -18.4595, 27.7471,  -27.3750, -85.3739, 40.6270,  -39.2609, -24.8645, 45.2326,
    -56.6830, 10.4099,  45.3931,   -69.4421, 28.6310,  45.0606,  -78.1369, 41.3210,  17.0778,  -22.4696, -81.3793,
    20.3420,  -33.0670, -23.0879,  17.9202,  -51.9830, 15.7123,  14.9159,  -68.0406, 36.4155,  12.5946,  -78.9575,
    49.5932,  7.4569,   -21.9225,  -79.2171, 2.0990,   -32.3352, -28.2736, -3.9275,  -50.1595, 7.2539,   -7.9765,
    -66.0658, 26.1811,  -8.7711,   -78.5253, 38.2895,  -2.4532,  -24.6924, -80.5279, -15.2679, -35.1095, -35.4490,
    -24.7150, -47.0335, -7.0120,   -29.8117, -58.5901, 5.6566,   -32.0271, -69.2831, 15.7944};

static std::map<int, int> JOINT_ID_BONE_DICT{{0, 0},   {1, 1},   {2, 2},   {3, 3},   {5, 4},   {6, 5},   {7, 6},
                                             {8, 7},   {10, 8},  {11, 9},  {12, 10}, {13, 11}, {15, 12}, {16, 13},
                                             {17, 14}, {18, 15}, {20, 16}, {21, 17}, {22, 18}, {23, 19}};

const static std::array<int, 21> VALID_JOINT_INDEX{0,  1,  2,  3,  4,  6,  7,  8,  9,  11, 12,
                                                   13, 14, 16, 17, 18, 19, 21, 22, 23, 24};

const static std::array<int, 25> KINTREE_PARENTS{-1, 0,  1, 2,  3,  0,  5,  6, 7,  8,  0,  10, 11,
                                                 12, 13, 0, 15, 16, 17, 18, 0, 20, 21, 22, 23};

static constexpr int STATIC_JOINT_NUM = 25;
Mat21_3f_t decode_hand_joints(float hand_scale, std::vector<float>& joint_angles) {
    // get joints
    const Eigen::Map<Eigen::Matrix<float, STATIC_JOINT_NUM, 3, Eigen::RowMajor>> template_joins(
        NIMBLE_TEMPLATE_JOINTS.data());
    const auto& root_joint = template_joins.row(0);
    Eigen::Matrix<float, 25, 3> real_joints =
        ((template_joins.rowwise() - root_joint) * (1 + hand_scale)).rowwise() + root_joint;
    std::vector<Eigen::Matrix4f> joint_transform_list(STATIC_JOINT_NUM);
    Eigen::Matrix4f root_j = Eigen::Matrix4f::Identity();
    root_j.block<3, 1>(0, 3) = real_joints.row(0);
    std::vector<Eigen::Matrix4f> hand_poses(25, Eigen::Matrix4f::Identity());
    hand_poses[0] = root_j;
    for (size_t i = 0; i < STATIC_JOINT_NUM - 1; i++) {
        int i_val_joint = i + 1;
        Eigen::Matrix3f joint_rot = Eigen::Matrix3f::Identity();
        if (JOINT_ID_BONE_DICT.count(i_val_joint) > 0) {
            int i_val_bone = JOINT_ID_BONE_DICT[i_val_joint];
            joint_rot =
                Eigen::Map<Eigen::Matrix<float, 3, 3, Eigen::RowMajor>>(joint_angles.data() + (i_val_bone - 1) * 9);
        }
        Eigen::Vector3f joint_j = real_joints.row(i_val_joint);
        int parent_id = KINTREE_PARENTS[i_val_joint];
        Eigen::Vector3f parent_j = real_joints.row(parent_id);
        Eigen::Matrix4f joint_rel_transform = Eigen::Matrix4f::Identity();
        joint_rel_transform.block<3, 3>(0, 0) = joint_rot;
        joint_rel_transform.block<3, 1>(0, 3) = joint_j - parent_j;
        hand_poses[i_val_joint] = hand_poses[parent_id] * joint_rel_transform;
    }
    Mat21_3f_t joints = Mat21_3f_t::Zero();
    auto rebuild_root = hand_poses[0].block<3, 1>(0, 3);
    for (int i = 0; i < VALID_JOINT_INDEX.size(); i++) {
        joints.row(i) = (hand_poses[VALID_JOINT_INDEX[i]].block<3, 1>(0, 3) - rebuild_root) / 1000.F;
    }
    return joints;
}

}  // namespace aisdk::algorithm