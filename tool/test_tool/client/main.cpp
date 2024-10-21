#include "grpc_client.h"
#include "load_dataset.h"
#include "picture_processer.h"
#include "report_result.h"
#include "test_config.h"
#include "video_processer.h"

int main(int argc, char** argv) {
    TestConfig config;
    if (argc != 2) {
        std::cout << "use example:  ./grpc_client_x86 ./input_config.json" << std::endl;
        return -1;
    }

    // 1.读取输入配置文件
    std::string input_config_path(argv[1]);
    int cfg_ok = config.LoadConfig(input_config_path);
    config.DumpConfig();
    if (0 != cfg_ok) {
        std::cout << "LoadConfig error" << std::endl;
        return -1;
    }

    // 2.连接服务器
    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 5000);
    args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5000);
    auto stubchannel = grpc::CreateCustomChannel(config.grpc_server_address, grpc::InsecureChannelCredentials(), args);
    if (nullptr == stubchannel) {
        std::cout << "connect grpc_server error" << std::endl;
        return -1;
    }

    // 2.启动grpc client端
    std::shared_ptr<AiClientImpl> grpc_aiclient = std::make_shared<AiClientImpl>(stubchannel);
    auto ret = grpc_aiclient->Status();
    if (0 != ret) {
        grpc_aiclient = nullptr;
        stubchannel = nullptr;
        return -1;
    }

    // 3.解析上传文件
    if (config.platform == ANDROID_PLATFORM && config.is_prepare_upload) {
        // x86 手动上传即可
        for (auto& iter : config.prepare_upload) {
            std::string src_file = std::get<0>(iter);
            std::string dst_file = std::get<1>(iter);
            auto ret = grpc_aiclient->Upload(config, src_file, dst_file);
            if (0 == ret) {
                std::cout << "prepare_upload [OK] src_file=" << src_file << std::endl;
                std::cout << "prepare_upload [OK] dst_file=" << dst_file << std::endl;
            }
        }
    }

    // 4.解析测试数据文件
    BenchmarkDataSets datasets;
    datasets.LoadDataSet(config);
    datasets.DumpDataSet(config);

    // 5. 启动server服务
    int Sdk_ok = -1;
    Sdk_ok = grpc_aiclient->ResetSdk();
    Sdk_ok |= grpc_aiclient->StartSdk(config);
    if (Sdk_ok) {
        grpc_aiclient = nullptr;
        stubchannel = nullptr;
        std::cout << "StartSdk error" << std::endl;
        return -1;
    }

    // 7.报告结果
    ReportResult reporter(config.out_dir, config.result_process);

    // 6.循环测试数据
    int work_error = 0;
    if (config.stream_type == PICTURE_DIR_STREAM || config.stream_type == PICTURE_LMDB_STREAM ||
        config.stream_type == PICTURE_GT_LMDB_STREAM) {
        PictureProcesser worker(grpc_aiclient);
        worker.BindBenchmarkDatas(datasets.benchmark_datas, config);
        worker.Start();
        while (1) {
            // 7.报告结果
            ToolsResults tmp;
            int ret = worker.GetReportResult(tmp);
            if (0 == ret) {
                reporter.show(tmp);
            } else if (-100 == ret || -99 == ret) {
                if (-99 == ret) {
                    work_error = -1;
                }
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        worker.WaitAndStop();

    } else if (config.stream_type == VIDEO_DIR_STREAM) {
        // VideoProcesser worker(grpc_aiclient);
        // worker.BindBenchmarkDatas(datasets.benchmark_datas->video_datas.l_video,
        //                           datasets.benchmark_datas->video_datas.r_video, config.max_used_frame_num);
        // worker.Start();
        // worker.WaitAndStop();
        // // 7.报告结果
        // reporter.show(worker.m_results);
    }

    datasets.UnLoadDataSet(config);
    // 8. 关闭server服务
    int Stop_ok = grpc_aiclient->StopSdk(config);
    grpc_aiclient = nullptr;
    stubchannel = nullptr;
    std::cout << "grpc_client exit!" << std::endl;
    if (work_error || Stop_ok) {
        return -1;
    }
    return 0;
}