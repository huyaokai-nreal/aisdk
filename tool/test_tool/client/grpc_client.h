#ifndef __GRPC_CLIENT_H__
#define __GRPC_CLIENT_H__

#include <google/protobuf/util/json_util.h>
#include <grpc++/grpc++.h>
#include <grpc/grpc.h>

#include "common.h"
#include "proto/nreal.ai.tool.grpc.pb.h"
#include "proto/nreal.ai.tool.pb.h"
#include "test_config.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

// grpc client 接口实现
class AiClientImpl {
   public:
    explicit AiClientImpl(std::shared_ptr<grpc::Channel>& channel) : m_stub(NrealAiTool::Engine::NewStub(channel)) {}
    ~AiClientImpl() = default;

   public:
    int Status();
    int Upload(TestConfig& tconfig, std::string& src_file, std::string& dst_file);
    int ResetSdk();
    int StartSdk(TestConfig& tconfig);
    int StopSdk(TestConfig& tconfig);
    int SendFrame(NrealAiTool::PipelineInferenceRequest& request);
    int RecvPipelineResult(uint64_t frame_id, NrealAiTool::GetPipelineResultReply* response);

   public:
    bool server_auto_cv;
    uint64_t session_id;

   private:
    std::unique_ptr<NrealAiTool::Engine::Stub> m_stub;
};

#endif