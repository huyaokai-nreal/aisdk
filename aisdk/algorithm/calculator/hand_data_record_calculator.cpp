#include <cstdint>
#include <utility>

#include "aisdk/algorithm/common/data_debug_record.h"
#include "aisdk/algorithm/internal_structs/data_record_struct_internal.h"
#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/algorithm/internal_structs/hand_gesture_struct_internal.h"
#include "aisdk/algorithm/internal_structs/headpose_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"

namespace aisdk::algorithm {

// node {
//   name: "HandDataRecord"
//   calculator: "HandDataRecordCalculator"
//   input_stream: "IMAGE_INPUT:image"
//   input_stream: "HEADPOSE_INPUT:head_pose"
//   input_stream: "DET_BBOX_OUTPUT:detection_output"
//   input_stream: "LANDMARK_OUTPUT:kpt2d"
//   input_stream: "LIFT_OUTPUT:kpt3d"
//   input_stream: "BLOCK_OUT:kpt3d_blocked"
//   input_stream: "GR_OUTPUT:gesture"
//   input_side_packet: "CAM_INFO_INPUT:cam_info"
//   input_stream_handler {
//     input_stream_handler: "ImmediateInputStreamHandler"
//   }
// }

class HandDataRecordState {
   public:
    Recordcache* FindCanExport(bool force) {
        for (auto iter = frame_datacache.begin(); iter != frame_datacache.end(); iter++) {
            // iter->second.m_nodestatus == NodeStatus::GESTURE_FINISH 当前帧被分析
            // iter->first < image_latest_time 新帧以及到达，但是上一帧还没完成(没detect到目标)
            if (force || iter->second.m_nodestatus == NodeStatus::GESTURE_FINISH || iter->first < image_latest_time) {
                AISDK_LOG_TRACE("[HandDataRecordState] FindCanExport {}", iter->first);
                return &iter->second;
            }
        }
        return nullptr;
    }

    void ClearHasExported(int64_t frame_timestamp) { frame_datacache.erase(frame_timestamp); }

    void SetLatestTime(int64_t frame_timestamp) { image_latest_time = frame_timestamp; }

    Recordcache* FindCache(int64_t frame_timestamp, bool is_add) {
        auto iter = frame_datacache.find(frame_timestamp);
        if (iter != frame_datacache.end()) {
            return &(iter->second);
        }

        if (is_add) {
            auto insert_iter = frame_datacache.insert(std::make_pair(frame_timestamp, Recordcache()));
            Recordcache* ret = &(insert_iter.first->second);
            ret->sequence_id = m_gsequence_inference_id++;
            ret->frame_timestamp = frame_timestamp;
            return ret;
        }
        return nullptr;
    }

   private:
    uint64_t m_gsequence_inference_id = 0;
    uint64_t m_gsequence_predict_id = 0;
    int64_t image_latest_time = 0;
    std::map<int64_t, Recordcache> frame_datacache;
};

class HandDataRecordCalculator : public xgraph::CalculatorBase {
   private:
    DataDebugRecord recorder;
    HandDataRecordState m_mgr;
    std::shared_ptr<base::BaseCameraModel> lcam_model_ = nullptr;
    std::shared_ptr<base::BaseCameraModel> rcam_model_ = nullptr;

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[HandDataRecordCalculator] GetContract start");

        cc->InputSidePackets()
            .Tag("CAM_INFO_INPUT")
            .Set<std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>,
                           std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        cc->Inputs().Tag("IMAGE_INPUT").Set<std::vector<Image>>();
        cc->Inputs().Tag("HEADPOSE_INPUT").Set<HeadPoseInternal>();
        cc->Inputs().Tag("DET_BBOX_OUTPUT").Set<DetOutputInternal>();
        cc->Inputs().Tag("LANDMARK_OUTPUT").Set<Kpt2dInternal>();
        cc->Inputs().Tag("LIFT_OUTPUT").Set<Kpt3dInternal>();
        cc->Inputs().Tag("BLOCK_OUT").Set<Kpt3dInternal>();
        cc->Inputs().Tag("GR_OUTPUT").Set<HandGestureInternal>();

        AISDK_LOG_TRACE("[HandDataRecordCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[HandDataRecordCalculator] Open complete.");
        const auto& cam_info = cc->InputSidePackets()
                                   .Tag("CAM_INFO_INPUT")
                                   .Get<std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>,
                                                  std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        lcam_model_ = cam_info.first;
        rcam_model_ = cam_info.second;
        recorder.InitDebugConfig();
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(HandDataRecordCalculator::Process);
#endif
        AISDK_LOG_TRACE("[HandDataRecordCalculator] Process start");
        bool is_any_input_close = false;
        bool is_debug_close = false;
        // 输入streamhandle是多输入，并且不是同步的，是及时响应类型的。
        // 意味着有输入就需要响应，多输入同时完成的动作，需要自己判断。
        if (recorder.CheckRealTimeDebugUnityButton(0)) {
            // 某个输入以及产生
            for (aisdk::xgraph::CollectionItemId id = cc->Inputs().BeginId(); id < cc->Inputs().EndId(); ++id) {
                auto& coll = cc->Inputs().Get(id);
                if (!coll.IsEmpty()) {
                    auto& package = coll.Value();
                    int64_t time_id = package.Timestamp().Value();
                    if (coll.Name() == "image") {
                        Recordcache* cache = m_mgr.FindCache(time_id, true);
                        if (cache) {
                            const auto& image_data = package.Get<std::vector<Image>>();
                            cache->m_nodestatus = NodeStatus::INPUT_IMAGE;
                            cache->detect_images.resize(2);
                            cache->detect_images[0].m_mat = image_data[0].m_mat.clone();
                            cache->detect_images[1].m_mat = image_data[1].m_mat.clone();
                            recorder.DebugImage(cache, image_data);
                        }
                        m_mgr.SetLatestTime(time_id);
                    } else if (coll.Name() == "head_pose") {
                        Recordcache* cache = m_mgr.FindCache(time_id, true);
                        if (cache) {
                            const auto& headpose_data = package.Get<HeadPoseInternal>();
                            cache->m_nodestatus = NodeStatus::INPUT_HEADPOSE;
                            recorder.DebugHeadpose(cache, headpose_data);
                        }
                    } else if (coll.Name() == "detection_output") {
                        Recordcache* cache = m_mgr.FindCache(time_id, false);
                        if (cache) {
                            const auto& detect_data = package.Get<DetOutputInternal>();
                            cache->m_nodestatus = NodeStatus::DETECT_FINISH;
                            cache->is_tracker_detect = !detect_data.det_flag;
                            cache->lhand_lcam_valid = detect_data.lhand_lcam_valid;
                            cache->lhand_rcam_valid = detect_data.lhand_rcam_valid;
                            cache->rhand_lcam_valid = detect_data.rhand_lcam_valid;
                            cache->rhand_rcam_valid = detect_data.rhand_rcam_valid;
                            // 检查状态
                            cache->lhand_valid = cache->lhand_lcam_valid && cache->lhand_rcam_valid;
                            cache->rhand_valid = cache->rhand_lcam_valid && cache->rhand_rcam_valid;
                            cache->lhand_status =
                                cache->lhand_valid ? ObjectStatus::NO_MISS : ObjectStatus::DETECT_MISS;
                            cache->rhand_status =
                                cache->rhand_valid ? ObjectStatus::NO_MISS : ObjectStatus::DETECT_MISS;
                            recorder.DebugDetect(cache, detect_data);
                        }
                    } else if (coll.Name() == "kpt2d") {
                        Recordcache* cache = m_mgr.FindCache(time_id, false);
                        if (cache) {
                            const auto& kpt2d_data = package.Get<Kpt2dInternal>();
                            cache->m_nodestatus = NodeStatus::RSN_FINISH;
                            cache->lhand_status = (cache->lhand_valid && !kpt2d_data.lhand_valid)
                                                      ? ObjectStatus::LANDMARK_MISS
                                                      : cache->lhand_status;
                            cache->rhand_status = (cache->rhand_valid && !kpt2d_data.rhand_valid)
                                                      ? ObjectStatus::LANDMARK_MISS
                                                      : cache->rhand_status;
                            cache->lhand_valid = kpt2d_data.lhand_valid;
                            cache->rhand_valid = kpt2d_data.rhand_valid;
                            recorder.DebugRsn(cache, kpt2d_data);
                        }
                    } else if (coll.Name() == "kpt3d") {
                        Recordcache* cache = m_mgr.FindCache(time_id, false);
                        if (cache) {
                            const auto& kpt3d_data = package.Get<Kpt3dInternal>();
                            cache->m_nodestatus = NodeStatus::LIFT_FINISH;
                            cache->lhand_status = (cache->lhand_valid && !kpt3d_data.lhand_valid)
                                                      ? ObjectStatus::LIFT_MISS
                                                      : cache->lhand_status;
                            cache->rhand_status = (cache->rhand_valid && !kpt3d_data.rhand_valid)
                                                      ? ObjectStatus::LIFT_MISS
                                                      : cache->rhand_status;
                            cache->lhand_valid = kpt3d_data.lhand_valid;
                            cache->rhand_valid = kpt3d_data.rhand_valid;

                            if (kpt3d_data.lhand_valid) {
                                cache->lhand_lcam_reproj_kpt2d = lcam_model_->world_to_window(kpt3d_data.lhand_kpt);
                                cache->lhand_rcam_reproj_kpt2d = rcam_model_->world_to_window(kpt3d_data.lhand_kpt);
                            }
                            if (kpt3d_data.rhand_valid) {
                                cache->rhand_lcam_reproj_kpt2d = lcam_model_->world_to_window(kpt3d_data.rhand_kpt);
                                cache->rhand_rcam_reproj_kpt2d = rcam_model_->world_to_window(kpt3d_data.rhand_kpt);
                            }
                            recorder.DebugLift(cache, kpt3d_data);
                        }
                    } else if (coll.Name() == "kpt3d_blocked") {
                        Recordcache* cache = m_mgr.FindCache(time_id, false);
                        if (cache) {
                            const auto& kpt3d_data = package.Get<Kpt3dInternal>();
                            cache->m_nodestatus = NodeStatus::MANO_FINISH;
                            cache->lhand_status = (cache->lhand_valid && !kpt3d_data.lhand_valid)
                                                      ? ObjectStatus::HARDRULE_MISS
                                                      : cache->lhand_status;
                            cache->rhand_status = (cache->rhand_valid && !kpt3d_data.rhand_valid)
                                                      ? ObjectStatus::HARDRULE_MISS
                                                      : cache->rhand_status;
                            cache->lhand_valid = kpt3d_data.lhand_valid;
                            cache->rhand_valid = kpt3d_data.rhand_valid;
                        }
                    } else if (coll.Name() == "gesture") {
                        Recordcache* cache = m_mgr.FindCache(time_id, false);
                        if (cache) {
                            const auto& gesture = package.Get<HandGestureInternal>();
                            cache->m_nodestatus = NodeStatus::GESTURE_FINISH;
                            recorder.DebugGestureReg(cache, gesture);
                        }
                    }
                    AISDK_LOG_TRACE("HandDataRecordCalculator name = {} id = {} time_id = {}\n",
                                    cc->Inputs().Get(id).Name().c_str(), id.value(), time_id);
                } else if (coll.IsDone()) {
                    is_any_input_close = true;
                    AISDK_LOG_ERROR("HandDataRecordCalculator name = {} id = {} IsDone\n",
                                    cc->Inputs().Get(id).Name().c_str(), id.value());
                }
            }

            if (false == is_any_input_close) {
                // 多输入中某些输入不满足 或者 多输入全部已经完成
                Recordcache* all_cache = m_mgr.FindCanExport(false);
                if (all_cache) {
                    recorder.DebugWholeInference(all_cache);
                    m_mgr.ClearHasExported(all_cache->frame_timestamp);
                }
            }
        } else {
            is_debug_close = true;
        }

        if (is_debug_close || is_any_input_close) {
            // 全部清除cache
            while (1) {
                Recordcache* all_cache = m_mgr.FindCanExport(true);
                if (all_cache) {
                    recorder.DebugWholeInference(all_cache);
                    m_mgr.ClearHasExported(all_cache->frame_timestamp);
                } else {
                    break;
                }
            }
        }
        AISDK_LOG_TRACE("[HandDataRecordCalculator] Process complete");

        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm