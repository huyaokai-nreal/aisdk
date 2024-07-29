#pragma once

#include <vector>
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include <json/json.h>

namespace aisdk::algorithm {

enum class NodeStatus {
    BUSY_DISCARD = -1,
    UNKNOWN = 0,
    INPUT_IMAGE = 1,
    INPUT_HEADPOSE = 2,
    DETECT_FINISH = 3,
    RSN_FINISH = 4,
    FILTER_FINISH = 5,
    LIFT_FINISH = 6,
    MANO_FINISH = 7,
    GLOBAL_FILTER_FINISH = 8,
    GESTURE_FINISH = 9,
    STDHAND_FINISH = 20,
};

enum class ObjectStatus { NO_MISS = 0, DETECT_MISS = 1, PF_MISS = 2, LANDMARK_MISS=3, LIFT_MISS=4, HARDRULE_MISS = 5};

struct Recordcache {
    uint64_t sequence_id = 0;
    int64_t frame_timestamp = 0;
    NodeStatus m_nodestatus = NodeStatus::UNKNOWN;
    std::vector<Image> detect_images; // 后续画图使用，必须copy

    bool is_tracker_detect = false;
    bool lhand_lcam_valid = false;
    bool lhand_rcam_valid = false;
    bool rhand_lcam_valid = false;
    bool rhand_rcam_valid = false;
    bool lhand_valid = false;
    bool rhand_valid = false;
    ObjectStatus lhand_status = ObjectStatus::NO_MISS;
    ObjectStatus rhand_status = ObjectStatus::NO_MISS;

    std::vector<Eigen::Vector2f> lhand_lcam_reproj_kpt2d;
    std::vector<Eigen::Vector2f> lhand_rcam_reproj_kpt2d;
    std::vector<Eigen::Vector2f> rhand_lcam_reproj_kpt2d;
    std::vector<Eigen::Vector2f> rhand_rcam_reproj_kpt2d;

    Json::Value export_root;  // debug_export
};

}  // namespace aisdk::algorithm