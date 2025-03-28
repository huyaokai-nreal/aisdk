#include <absl/memory/memory.h>
#include <absl/status/status.h>
#include <aisdk/algorithm/internal_structs/kpt3d_struct_internal.h>
#include <aisdk/algorithm/internal_structs/headpose_struct_internal.h>
#include <aisdk/algorithm/internal_structs/det_struct_internal.h>
#include <aisdk/algorithm/calculator/xgraph_service_utils.h>
#include <aisdk/algorithm/common/nrnet_define.h>
#include <aisdk/task/handtracking/base_xgraph.h>
#include <aisdk/xgraph/xgraph.h>
#include <aisdk/base/camera_model.h>
#include <mediapipe/framework/packet.h>
#include <mediapipe/framework/timestamp.h>
#include <map>
#include <filesystem>
#include "aisdk/task/handtracking/handtracking_calculators_register.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
using namespace aisdk;

TEST_CASE("test HandDetTrackCalculator with model file") {
    task::TriggerGloalGraphCalculatorsConstruct();

    constexpr char kTestNodeConfig[] = R"(
        calculator: "HandDetTrackCalculator"
        input_stream: "IMAGE_INPUT:image"
        input_side_packet: "CAM_INFO_INPUT:cam_info"
        input_stream: "HEADPOSE:head_pose"
        output_stream: "DET_BBOX_OUTPUT:detection_output"
        output_stream: "IMAGE_OUTPUT:image_out"
        output_stream: "HEADPOSE_OUTPUT:head_pose_out"
        input_stream_handler {
            input_stream_handler: "FixedSizeInputStreamHandler"
            options {
                [mediapipe.FixedSizeInputStreamHandlerOptions.ext] {
                    trigger_queue_size: 2
                    target_queue_size: 1
                    fixed_min_size: false
                }
            }
        }
        node_options: {
            [type.googleapis.com/aisdk.HandDetTrackCalculatorOptions] {
                model_name: "detect"
                enable_track: true
            }
        }
    )";

    // 解析配置前验证模型文件存在（避免因路径错误导致崩溃）
#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
    const std::string model_path = "./det_flora_250224_yolov5_quantized.dlc";
#endif
#if defined(Xrlinux)
    const std::string model_path = "./det_tiny_240415.h2.s.B.batch1.ar9401.npubin";
#endif
    REQUIRE(std::filesystem::exists(model_path));  // 需要包含<filesystem>头文件

    // 解析节点配置
    xgraph::CalculatorGraphConfig::Node node_config = 
        xgraph::ParseTextProtoOrDie<xgraph::CalculatorGraphConfig::Node>(kTestNodeConfig);

    // 创建计算器运行器
    xgraph::CalculatorRunner runner(node_config);

    // 构造输入数据 --------------------------------
    
    // 1. 构造输入图像信息
    const std::string image_path = "./test_hand_image.jpg";  // 测试图片路径
    REQUIRE(std::filesystem::exists(image_path));  // 确保文件存在
    cv::Mat cv_image = cv::imread(image_path, cv::IMREAD_COLOR);  //使用opencv读取图像
    REQUIRE(!cv_image.empty());  // 确保加载成功
    aisdk::algorithm::Image img(cv_image);
    auto image_frame = absl::make_unique<aisdk::algorithm::Image>(img);
    
    // 2. 模拟相机参数（通过SidePacket传递）
    //aisdk::algorithm::CameraParams camera;
    //auto camera_info = ConvertCameraInfo(camera);
    //auto camera_model = ConvertCameraModel(camera_info);
    std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>> camera_model;
    //要修改，构建camera_model的具体内容
    auto cam_info_container = 
        absl::make_unique<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>(std::move(camera_model));
    
    // 3. 模拟头部姿态数据
    auto head_pose = absl::make_unique<aisdk::algorithm::HeadPoseInternal>();
    //要修改，构建HeadPoseInternal
    // head_pose->rotation = {0.1, 0.2, 0.3};
    // head_pose->translation = {0.5, 0.5, 2.0};

    // 添加输入到计算器 ----------------------------
    
    // 图像输入流（时间戳=1）
    runner.MutableInputs()
        ->Tag("IMAGE_INPUT")
        .packets.push_back(
            mediapipe::Adopt(image_frame.release())
                .At(mediapipe::Timestamp(1))
        );

    // 头部姿态输入流（时间戳同步）
    runner.MutableInputs()
        ->Tag("HEADPOSE")
        .packets.push_back(
            mediapipe::Adopt(head_pose.release())
                .At(mediapipe::Timestamp(1))
        );

    // 相机参数通过SidePacket传递（无时间戳）
    runner.MutableSidePackets()
        ->Tag("CAM_INFO_INPUT") = 
            mediapipe::Adopt(cam_info_container.release());

    // 运行计算器并验证结果 -------------------------------------
    absl::Status status = runner.Run();
    
    // 额外验证模型加载状态（通过计算器日志或自定义输出）
    if (!status.ok()) {
        std::cerr << "Calculator初始化失败,可能原因: \n"
                  << "1. 模型路径不正确（当前路径: " 
                  << std::filesystem::absolute(model_path) << ")\n"
                  << "2. 模型文件格式不兼容\n"
                  << "错误详情: " << status.message() << std::endl;
    }
    CHECK(status.ok());  // 确保模型加载成功

    // 验证输出 ------------------------------------
    
    // 1. 检测框输出（至少应有空结果）
    const auto& det_outputs = runner.Outputs().Tag("DET_BBOX_OUTPUT").packets;
    CHECK(det_outputs.size() == 1);  // 应有1个输出包
    const auto& bboxes = det_outputs[0].Get<aisdk::algorithm::DetOutputInternal>();
    // CHECK(bboxes.detections.size() >= 0);  // 可能无检测但结构存在

    // 2. 图像输出（应存在且尺寸匹配）
    const auto& image_outputs = runner.Outputs().Tag("IMAGE_OUTPUT").packets;
    CHECK(image_outputs.size() == 1);
    const auto& out_image = image_outputs[0].Get<aisdk::algorithm::Image>();
    // CHECK(out_image.Width() == 640);
    // CHECK(out_image.Height() == 480);

    // 3. 头部姿态透传（应与输入一致）
    const auto& head_pose_outputs = runner.Outputs().Tag("HEADPOSE_OUTPUT").packets;
    CHECK(head_pose_outputs.size() == 1);
    const auto& out_head_pose = head_pose_outputs[0].Get<aisdk::algorithm::HeadPoseInternal>();
    // CHECK(out_head_pose.rotation == head_pose->rotation);  // 需重载==操作符
    // CHECK(out_head_pose.translation == head_pose->translation);
}
