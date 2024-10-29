#include <google/protobuf/util/json_util.h>
//#include <grpc++/ext/proto_server_reflection_plugin.h>
#include <grpc++/grpc++.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc/grpc.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include "aisdk/base/file.h"
#include "hand_tracking.h"
#include "json/json.h"
#include "proto/nreal.ai.tool.grpc.pb.h"
#include "proto/nreal.ai.tool.pb.h"

using grpc::Server;
using grpc::Status;

static std::string server_proto_version = "NrealAiTool 20230119.A02";
static std::string server_defaut_dir = "./";
static std::set<std::string> support_sdk = {"handTracking"};
static std::atomic_int QuitFlag = 0;

void protoToJson(const google::protobuf::Message& proto) {
    std::string json;
    google::protobuf::util::MessageToJsonString(proto, &json);
    std::cout << json << std::endl;
}

class AIServiceImpl final : public NrealAiTool::Engine::Service {
   public:
    AIServiceImpl() = default;
    virtual ~AIServiceImpl() {
        if (m_handTracking_sdk_impl) {
            m_handTracking_sdk_impl = nullptr;
            DestroyHandTrackingInstance();
        }
    }
    grpc::Status Status(grpc::ServerContext* context, const google::protobuf::Empty* request,
                        NrealAiTool::StatusReply* response) override {
        std::cout << "grpc server [Status] request" << std::endl;

        response->mutable_system_status()->append(server_proto_version);
        response->mutable_system_status()->append("\n");
        if (0 != m_handTracking_sdk_sessionid) {
            response->mutable_system_status()->append(std::string("handTracking is running, sessionid=") +
                                                      std::to_string(m_handTracking_sdk_sessionid));
            response->mutable_system_status()->append("\n");
        }
        return Status::OK;
    }

    grpc::Status Upload(grpc::ServerContext* context, const NrealAiTool::UploadRequest* request,
                        NrealAiTool::UploadReply* response) override {
        std::cout << "grpc server [Upload] request" << std::endl;

        if (0 == request->sdk_name().length() || 0 == request->file_name().length() ||
            0 == request->file_content().length() || support_sdk.find(request->sdk_name()) == support_sdk.end()) {
            response->set_ret_code(-1);
            response->mutable_error_message()->append("UploadRequest param check error");
            return Status::OK;
        }

        std::string save_dir = server_defaut_dir + request->sdk_name();
        aisdk::base::CreateDir(save_dir);
        save_dir = save_dir + "/" + request->file_name();

        if (0 == aisdk::base::WriteToFile(save_dir, request->file_content())) {
            response->set_ret_code(0);
            return Status::OK;
        }              
        response->set_ret_code(-3);
        response->mutable_error_message()->append("open file_name error");
        return Status::OK;
       
    }

    grpc::Status ResetSdk(grpc::ServerContext* context, const google::protobuf::Empty* request,
                          NrealAiTool::ResetSdkReply* response) override {
        std::cout << "grpc server [ResetSdk] request" << std::endl;

        std::lock_guard<std::mutex> guard(m_lock);
        if (m_handTracking_sdk_sessionid && m_handTracking_sdk_impl) {
            auto ret = m_handTracking_sdk_impl->StopSdk();
            if (0 == ret) {
                m_handTracking_sdk_impl = nullptr;
                m_handTracking_sdk_sessionid = 0;
                DestroyHandTrackingInstance();
                response->set_ret_code(0);
            } else {
                response->set_ret_code(-1);
                response->mutable_error_message()->append("handTracking StopSdk error");
            }
        } else {
            response->set_ret_code(0);
        }
        return Status::OK;
    }

    grpc::Status StartSdk(grpc::ServerContext* context, const NrealAiTool::StartSdkRequest* request,
                          NrealAiTool::StartSdkReply* response) override {
        std::cout << "grpc server [StartSdk] request" << std::endl;

        if (request->sdk_name() == "handTracking") {
            std::lock_guard<std::mutex> guard(m_lock);
            if (nullptr == m_handTracking_sdk_impl) {
                auto sdk = GetHandTrackingInstance();
                // you can convert a google::protobuf::Map to a standard map
                // Note that this will make a deep copy of the entire map.
                std::map<std::string, std::string> standard_config_params(request->config_params().begin(),
                                                                          request->config_params().end());
                SdkConfigParams sdk_config;
                sdk_config.input_width = request->input_width();
                sdk_config.input_height = request->input_height();
                sdk_config.input_datatype = (uint32_t)request->input_datatype();
                sdk_config.stream_sampling_fps = request->stream_sampling_fps();
                sdk_config.simulation_sendframe_fps = request->simulation_sendframe_fps();
                sdk_config.simulation_getresult_fps = request->simulation_getresult_fps();
                sdk_config.prediction_forward_ms = request->prediction_forward_ms();
                sdk_config.max_cache_fn = request->max_cache_fn();
                sdk_config.config_params = std::move(standard_config_params);
                auto ret = sdk->StartSdk(sdk_config);
                if (0 == ret) {
                    m_handTracking_sdk_impl = sdk;
                    m_handTracking_sdk_sessionid = (uint64_t)m_handTracking_sdk_impl.get();
                    response->set_ret_code(0);
                    response->set_session_id(m_handTracking_sdk_sessionid);
                    // 目前写死
                    response->set_server_auto_cv(false);
                } else {
                    sdk = nullptr;
                    DestroyHandTrackingInstance();
                    response->set_ret_code(-3);
                    response->mutable_error_message()->append("handTracking StartSdk failure");
                }
            } else {
                response->set_ret_code(-2);
                response->mutable_error_message()->append("handTracking started");
            }
        } else {
            response->set_ret_code(-1);
            response->mutable_error_message()->append("no support sdk_name");
        }

        protoToJson(*response);
        return Status::OK;
    }

    grpc::Status StopSdk(grpc::ServerContext* context, const NrealAiTool::StopSdkRequest* request,
                         NrealAiTool::StopSdkReply* response) override {
        std::cout << "grpc server [StopSdk] request" << std::endl;

        std::lock_guard<std::mutex> guard(m_lock);
        if (request->sdk_name() == "handTracking" && request->session_id() == m_handTracking_sdk_sessionid &&
            m_handTracking_sdk_impl) {
            auto ret = m_handTracking_sdk_impl->StopSdk();
            if (0 == ret) {
                m_handTracking_sdk_impl = nullptr;
                m_handTracking_sdk_sessionid = 0;
                DestroyHandTrackingInstance();
                response->set_ret_code(0);
            } else {
                response->set_ret_code(-2);
                response->mutable_error_message()->append("handTracking StopSdk error");
            }
        } else {
            response->set_ret_code(-1);
            response->mutable_error_message()->append("StopSdk param check error");
        }
        return Status::OK;
    }

    grpc::Status PipelineInference(grpc::ServerContext* context, const NrealAiTool::PipelineInferenceRequest* request,
                                   NrealAiTool::PipelineInferenceReply* response) override {
        if (m_handTracking_sdk_sessionid != request->session_id() ||
            (false == request->is_eof() &&
             (0 == request->left_camera_frame().length() || 0 == request->right_camera_frame().length()))) {
            response->set_ret_code(-1);
            response->mutable_error_message()->append("PipelineInference param check error");
        } else {
            std::shared_ptr<StreamData> data = nullptr;
            if (false == request->is_eof()) {
                data = std::make_shared<StreamData>();
                data->frame_id = request->frame_id();
                data->nano_time = request->nano_time();
                data->width = request->width();
                data->height = request->height();
                auto lframe = request->left_camera_frame();
                auto rframe = request->right_camera_frame();
                // data->left_frame = std::move(lframe);
                // data->right_frame = std::move(rframe);
                auto lens1 = lframe.size();
                auto lens2 = rframe.size();
                data->left_right_frame.resize(lens1 + lens2);
                memcpy((char*)data->left_right_frame.data(), lframe.data(), lens1);
                memcpy((char*)data->left_right_frame.data() + lens1, rframe.data(), lens2);
                for (uint32_t i = 0; i < request->head_pose().transform_size() && i < 7; i++) {
                    data->headpose[i] = request->head_pose().transform(i);
                }
                data->token = request->token();
            }

            auto ret = m_handTracking_sdk_impl->SendStream(data);
            if (0 == ret) {
                response->set_ret_code(0);
            } else if (-100 == ret) {
                response->set_ret_code(-100);
                response->mutable_error_message()->append("PipelineInference SendStream waiting");
            } else {
                response->set_ret_code(-2);
                response->mutable_error_message()->append("PipelineInference SendStream error");
            }
        }

        return Status::OK;
    }

    grpc::Status GetPipelineResult(grpc::ServerContext* context, const NrealAiTool::GetPipelineResultRequest* request,
                                   NrealAiTool::GetPipelineResultReply* response) override {
        if (m_handTracking_sdk_sessionid != request->session_id()) {
            response->set_ret_code(-1);
            response->mutable_error_message()->append("GetPipelineResult param check error");
        } else {
            std::shared_ptr<StreamResult> result;
            int ret = m_handTracking_sdk_impl->RecvResult(request->frame_id(), result);
            if (0 == ret) {
                // google::protobuf::Map<int32, int32> weight(standard_map.begin(),
                // standard_map.end());
                if (result->hand_tracking_data.size()) {
                    response->set_result_type(NrealAiTool::ResultType::HANDTRACKING_PREDICTION_INFO);
                    (*response->mutable_result_info())["HandTrackingData"] = std::move(result->hand_tracking_data);
                } else if (result->profiling_exec_data.size()) {
                    response->set_frame_id(result->frame_id);
                    response->set_token(result->token);
                    response->set_result_type(NrealAiTool::ResultType::HANDTRACKING_PROFILING_INFO);
                    (*response->mutable_result_info())["ProfilingExecData"] = std::move(result->profiling_exec_data);
                }
                response->set_ret_code(0);
            } else if (-100 == ret) {
                response->set_ret_code(-100);
                response->mutable_error_message()->append("PipelineInference RecvResult over");
            } else {
                response->set_ret_code(-2);
                response->mutable_error_message()->append("GetPipelineResult RecvResult error");
            }
        }
        return Status::OK;
    }

    grpc::Status SetServiceFeature(grpc::ServerContext* context, const NrealAiTool::SetServiceFeatureRequest* request,
                                   NrealAiTool::SetServiceFeatureReply* response) override {
        std::cout << "grpc server [SetServiceFeature] request" << std::endl;
        return Status::OK;
    }

    grpc::Status GetServiceActualInfo(grpc::ServerContext* context,
                                      const NrealAiTool::GetServiceActualInfoRequest* request,
                                      NrealAiTool::GetServiceActualInfoReply* response) override {
        std::cout << "grpc server [GetServiceActualInfo] request" << std::endl;
        return Status::OK;
    }

   private:
    std::mutex m_lock;
    uint64_t m_handTracking_sdk_sessionid = 0;
    std::shared_ptr<HandTrackingSdk> m_handTracking_sdk_impl = nullptr;
};

class RpcServer {
   private:
    AIServiceImpl m_ai_service;
    std::unique_ptr<grpc::Server> m_grpc;
   public:
    int Start(const std::string& server_address) {
        // grpc::reflection::InitProtoReflectionServerBuilderPlugin();

        grpc::ServerBuilder builder;
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS,
                                   1);  //默认为0，在没有rpc待处理的情况下，不允许发PING帧
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);  //默认7200000，两个小时后发送PING帧
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS,
                                   10000);  //默认20000，20秒后如果没收到PING ACK，就重发PING
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA,
                                   10);  //默认累计发了2个PING帧之后，必须发送一次带数据的帧才能继续发PING帧
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PING_STRIKES,
                                   5);  //默认为2，最多重发2次如果对方不响应，就断开连接
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.SetMaxReceiveMessageSize(50 * 1024 * 1024);
        builder.SetMaxSendMessageSize(20 * 1024 * 1024);
        grpc::ResourceQuota resource;
        resource.SetMaxThreads(10);
        builder.SetResourceQuota(resource);

        builder.RegisterService(&m_ai_service);
        m_grpc = builder.BuildAndStart();
        std::cout << "grpc server started" << std::endl;
        return 0;
    }

    int Stop() const {
        m_grpc->Shutdown();
        m_grpc->Wait();
        std::cout << "grpc server Stoped" << std::endl;
        return 0;
    }
};

void SignalHandler(int sig) {
    std::cout << "SignalHandler sig=" << sig << std::endl;
    QuitFlag = 1;
    exit(255);
}

int main(int argc, char** argv) {
    signal(SIGTERM, SignalHandler);
    signal(SIGINT, SignalHandler);
    signal(SIGABRT, SignalHandler);
    signal(SIGBUS, SignalHandler);
    signal(SIGSEGV, SignalHandler);

    auto rpcservice = std::make_unique<RpcServer>();
    rpcservice->Start("0.0.0.0:50051");
    while (1) {
        sleep(1);
        if (QuitFlag) { break;
}
    }
    rpcservice->Stop();
    rpcservice = nullptr;
    std::cout << "grpc_server exit!" << std::endl;
    return 0;
}