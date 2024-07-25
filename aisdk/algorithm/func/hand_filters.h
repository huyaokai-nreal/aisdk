#pragma once

#include <utility>
#include <vector>
#include <set>

#include "../common/NR_Seq_Manager.h"
namespace aisdk::algorithm {

class HandFilters final {
   public:
    HandFilters(std::string glasses_type="flora"): glasses_type_(std::move(glasses_type)){};
    ~HandFilters(){};
    bool init();
    void kpt_seq_3d_filter(int hand_side, std::vector<Vec3f_t>& point3d);
    bool reset(int hand_side);
   private:
    std::shared_ptr<SeqManager3D> m_seq3d_lhand;
    std::shared_ptr<SeqManager3D> m_seq3d_rhand;

    std::shared_ptr<SeqManager3D> m_seq3d_palm_lhand;
    std::shared_ptr<SeqManager3D> m_seq3d_palm_rhand;

    std::string glasses_type_;
    #if defined(ENABLE_OPENXR_HANDJOINT_FORMAT)
    int PalmKeypointNum = 10;
    std::set<uint32_t> PalmKeypointSet{1,5,9,13,17,21,22,23,24,25};
    #else
    int PalmKeypointNum = 7;
    std::set<uint32_t> PalmKeypointSet{1,5,9,13,17,21,22};
    #endif
};

}  // namespace aisdk::algorithm