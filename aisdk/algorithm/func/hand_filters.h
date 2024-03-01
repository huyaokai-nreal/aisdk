#pragma once

#include <vector>

#include "../common/NR_Seq_Manager.h"

class HandFilters final {
   public:
    HandFilters(){};
    ~HandFilters(){};
    bool init();
    void kpt_seq_3d_filter(int hand_side, std::vector<cv::Vec3f>& point3d);

   private:
    std::shared_ptr<aisdk::algorithm::SeqManager3D> m_seq3d_lhand;
    std::shared_ptr<aisdk::algorithm::SeqManager3D> m_seq3d_rhand;

    std::shared_ptr<aisdk::algorithm::SeqManager3D> m_seq3d_palm_lhand;
    std::shared_ptr<aisdk::algorithm::SeqManager3D> m_seq3d_palm_rhand;

    std::shared_ptr<aisdk::algorithm::SeqManager3D> m_seq3d_center_lhand;
    std::shared_ptr<aisdk::algorithm::SeqManager3D> m_seq3d_center_rhand;
};
