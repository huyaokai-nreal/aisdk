#pragma once

#include <utility>
#include <vector>
#include <set>

#include "../common/NR_Seq_Manager.h"
#include "aisdk/base/type.h"
namespace aisdk::algorithm {

class HandFilters final {
   public:
    explicit HandFilters(std::string glasses_type = "flora") : glasses_type_(std::move(glasses_type)){};
    ~HandFilters(){};
    bool init();
    std::vector<Vec3f_t> process(int hand_side, const std::vector<Vec3f_t>& point3d);
    bool reset(int hand_side);
    void set_filter_param(const OneEuroParams& palm_param, const OneEuroParams& finger_param) {
        // finger
        m_seq3d_lhand = std::make_shared<SeqManager3D>(15, finger_param);
        m_seq3d_rhand = std::make_shared<SeqManager3D>(15, finger_param);
        // palm
        m_seq3d_palm_lhand = std::make_shared<SeqManager3D>(7, palm_param);
        m_seq3d_palm_rhand = std::make_shared<SeqManager3D>(7, palm_param);
    }

   private:
    std::shared_ptr<SeqManager3D> m_seq3d_lhand;
    std::shared_ptr<SeqManager3D> m_seq3d_rhand;

    std::shared_ptr<SeqManager3D> m_seq3d_palm_lhand;
    std::shared_ptr<SeqManager3D> m_seq3d_palm_rhand;

    std::string glasses_type_;
};

}  // namespace aisdk::algorithm