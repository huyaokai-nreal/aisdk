#pragma once

#include <list>
#include <mutex>

#include "mediapipe/framework/calculator_framework.h"
#include "nrcore_pipeline.h"

namespace aisdk::algorithm {

class OutputCache {
   public:
    std::mutex m_lock;
    std::list<mediapipe::Packet> m_packs;
};

class MediaPipeGraph : public PipeGraphImpl {
   public:
    MediaPipeGraph() : PipeGraphImpl() {}
    virtual ~MediaPipeGraph() {}

    aisdk::algorithm::Status Init(aisdk::xengine::DlSymFuncs &funcs, aisdk::xengine::PipelineConfig &config,
                                  CameraParams &camera);
    aisdk::algorithm::Status Start();
    aisdk::algorithm::Status Stop();

   public:
    std::unique_ptr<mediapipe::CalculatorGraph> m_calculator_graph;
    std::vector<std::string> m_input_stream_name;
    std::vector<std::string> m_output_stream_name;
    std::map<std::string, std::shared_ptr<OutputCache>> m_output_stream_cache;
};

}  // namespace aisdk::algorithm
