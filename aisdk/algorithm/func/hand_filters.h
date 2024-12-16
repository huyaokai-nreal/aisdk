#pragma once

#include <utility>
#include <vector>
#include <set>

#include "../common/NR_Seq_Manager.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/base/type.h"
namespace aisdk::algorithm {

class HandFilters final {
   public:
    explicit HandFilters(std::string glasses_type = "flora") : glasses_type_(std::move(glasses_type)){};
    ~HandFilters(){};
    bool init();
    std::vector<Vec3f_t> process(int hand_side, const std::vector<Vec3f_t>& point3d);
    bool reset(int hand_side);
    void set_filter_param(const OneEuroParams& palm_params, const OneEuroParams& index_finger_params, const OneEuroParams& other_finger_params) {
    // index finger default
    m_seq3d_lindex = std::make_shared<SeqManager3D>(kIndexFingerIndexSet.size(), index_finger_params);
    m_seq3d_rindex = std::make_shared<SeqManager3D>(kIndexFingerIndexSet.size(), index_finger_params);

    // other finger
    m_seq3d_lhand = std::make_shared<SeqManager3D>(12, other_finger_params);
    m_seq3d_rhand = std::make_shared<SeqManager3D>(12, other_finger_params);
    // palm
    m_seq3d_palm_lhand = std::make_shared<SeqManager3D>(kPalmKeypointIndexSet.size(), palm_params);
    m_seq3d_palm_rhand = std::make_shared<SeqManager3D>(kPalmKeypointIndexSet.size(), palm_params);
    }

   private:
    std::shared_ptr<SeqManager3D> m_seq3d_lindex;
    std::shared_ptr<SeqManager3D> m_seq3d_rindex;
    std::shared_ptr<SeqManager3D> m_seq3d_lhand;
    std::shared_ptr<SeqManager3D> m_seq3d_rhand;

    std::shared_ptr<SeqManager3D> m_seq3d_palm_lhand;
    std::shared_ptr<SeqManager3D> m_seq3d_palm_rhand;

    std::string glasses_type_;
};

}  // namespace aisdk::algorithm