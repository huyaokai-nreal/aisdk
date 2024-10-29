#include "grpc_client.h"

#include "aisdk/base/file.h"

void protoToJson(const google::protobuf::Message& proto) {
    std::string json;
    google::protobuf::util::MessageToJsonString(proto, &json);
    std::cout << json << std::endl;
}

int AiClientImpl::Status() {
    grpc::ClientContext context;
    google::protobuf::Empty request;
    NrealAiTool::StatusReply response;
    grpc::Status ret = m_stub->Status(&context, request, &response);
    if (false == ret.ok()) {
        std::cout << "[Status] " << ret.error_code() << ";" << ret.error_message() << std::endl;
        return -1;
    }

    std::cout << "[server system_status] " << response.system_status() << std::endl;
    return 0;
}

int AiClientImpl::Upload(TestConfig& tconfig, std::string& src_file, std::string& dst_file) {
    grpc::ClientContext context;
    NrealAiTool::UploadRequest request;
    request.mutable_sdk_name()->append(tconfig.sdk_name);
    request.mutable_file_name()->append(dst_file);
    std::string file_content;
    aisdk::base::ReadFromFile(src_file, file_content);
    *request.mutable_file_content() = std::move(file_content);
    NrealAiTool::UploadReply response;
    grpc::Status ret = m_stub->Upload(&context, request, &response);
    if (false == ret.ok()) {
        std::cout << "[Upload] " << ret.error_code() << ";" << ret.error_message() << std::endl;
        return -1;
    }

    return 0;
}

int AiClientImpl::ResetSdk() {
    grpc::ClientContext context;
    google::protobuf::Empty request;
    NrealAiTool::ResetSdkReply response;
    grpc::Status ret = m_stub->ResetSdk(&context, request, &response);
    if (false == ret.ok()) {
        std::cout << "[ResetSdk] " << ret.error_code() << ";" << ret.error_message() << std::endl;
        return -1;
    }

    return 0;
}

int AiClientImpl::StartSdk(TestConfig& tconfig) {
    std::string camera_param_content;
    aisdk::base::ReadFromFile(tconfig.camera_param, camera_param_content);

    grpc::ClientContext context;
    NrealAiTool::StartSdkRequest request;
    request.mutable_sdk_name()->append(tconfig.sdk_name);
    request.set_input_width(tconfig.input_width);
    request.set_input_height(tconfig.input_height);
    request.set_input_datatype(NrealAiTool::DataType::GRAY_8UC1);
    request.set_stream_sampling_fps(tconfig.stream_sampling_fps);
    request.set_simulation_sendframe_fps(tconfig.send_frame_fps);
    request.set_simulation_getresult_fps(tconfig.recv_result_fps);
    request.set_prediction_forward_ms(tconfig.prediction_forward_ms);
    request.set_max_cache_fn(tconfig.max_cache_frame_num);
    (*request.mutable_config_params())["plugin_so"] = tconfig.sdk_plugin_so;
    (*request.mutable_config_params())["camera_param"] = camera_param_content;
    if (tconfig.is_has_highlevel_option) {
        (*request.mutable_config_params())["highlevel_option"] = tconfig.highlevel_option_json;
    }
    NrealAiTool::StartSdkReply response;
    grpc::Status ret = m_stub->StartSdk(&context, request, &response);
    if (false == ret.ok()) {
        std::cout << "[StartSdk] " << ret.error_code() << ";" << ret.error_message() << std::endl;
        return -1;
    }

    if (0 != response.ret_code()) {
        std::cout << "[StartSdk] " << response.ret_code() << ";" << response.error_message() << std::endl;
        return -2;
    }

    protoToJson(response);
    server_auto_cv = response.server_auto_cv();
    session_id = response.session_id();
    return 0;
}

int AiClientImpl::StopSdk(TestConfig& tconfig) {
    grpc::ClientContext context;
    NrealAiTool::StopSdkRequest request;
    request.mutable_sdk_name()->append(tconfig.sdk_name);
    request.set_session_id(session_id);
    NrealAiTool::StopSdkReply response;
    grpc::Status ret = m_stub->StopSdk(&context, request, &response);
    if (false == ret.ok()) {
        std::cout << "[StopSdk] " << ret.error_code() << ";" << ret.error_message() << std::endl;
        return -1;
    }

    if (0 != response.ret_code()) {
        std::cout << "[StopSdk] " << response.ret_code() << ";" << response.error_message() << std::endl;
        return -2;
    }
    return 0;
}

int AiClientImpl::SendFrame(NrealAiTool::PipelineInferenceRequest& request) {
    grpc::ClientContext context;
    NrealAiTool::PipelineInferenceReply response;
    grpc::Status ret = m_stub->PipelineInference(&context, request, &response);
    if (false == ret.ok()) {
        std::cout << "[PipelineInference] " << ret.error_code() << ";" << ret.error_message() << std::endl;
        return -1;
    }

    if (-100 == response.ret_code()) {
        // std::cout << "[PipelineInference] " << response.ret_code() << ";" << response.error_message() << std::endl;
        return -100;
    }

    if (0 != response.ret_code()) {
        // std::cout << "[PipelineInference] "
        //           << response.ret_code() << ";"
        //           << response.error_message()
        //           << std::endl;
        return -2;
    }

    return 0;
}

int AiClientImpl::RecvPipelineResult(uint64_t frame_id, NrealAiTool::GetPipelineResultReply* response) {
    grpc::ClientContext context;
    NrealAiTool::GetPipelineResultRequest request;
    request.set_session_id(session_id);
    request.set_frame_id(frame_id);
    grpc::Status ret = m_stub->GetPipelineResult(&context, request, response);
    if (false == ret.ok()) {
        std::cout << "[GetPipelineResult] " << ret.error_code() << ";" << ret.error_message() << std::endl;
        return -1;
    }

    if (-100 == response->ret_code()) {
        // std::cout << "[GetPipelineResult] " << response->ret_code() << ";" << response->error_message() << std::endl;
        return -100;
    }

    if (0 != response->ret_code()) {
        // std::cout << "[GetPipelineResult] "
        //           << response->ret_code() << ";"
        //           << response->error_message()
        //           << std::endl;
        return -2;
    }

    return 0;
}