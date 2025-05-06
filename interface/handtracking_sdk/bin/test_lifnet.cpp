#include "aisdk/xengine/nr_mnn_header.h"

#include <iostream>

using std::cout;
using std::endl;

int main(int argc, char** argv)
{
    cout << "start log recording" << endl;

    // 配置参数
    const std::string model_path = "liftnimble_res26sw_250316_reproj_ebeff3.mnn";
    const std::string input_dir = "input_3d/";
    const std::string output_dir = "output_mnn/";
    
    // 输入配置（根据实际模型调整）
    const std::vector<int> feat_shape = {1, 21, 5, 1};  // NCHW格式
    const std::vector<int> mem_in_shape = {1, 105, 1, 1};
    
    // 初始化MNN
    cout << "begin to init mnn" << endl;
    MNN::ScheduleConfig config;
    config.type = MNN_FORWARD_CPU;  // 使用CPU执行
    MNN::BackendConfig backend_config;
    backend_config.precision = MNN::BackendConfig::Precision_High;
    config.backendConfig = &backend_config;

    // 加载模型
    cout << "begin to load model" << endl;
    auto interpreter = std::shared_ptr<MNN::Interpreter>(
        MNN::Interpreter::createFromFile(model_path.c_str()),
        [](MNN::Interpreter* p) { if(p) p->releaseModel(); }
    );

    cout << "begin to create session"  << endl;
    auto session = interpreter->createSession(config);
    
    // 获取输入输出张量信息
    cout << "begin to get session input" << endl;
    auto feat_input = interpreter->getSessionInput(session, "feat");    // 输入名称需确认
    auto mem_input = interpreter->getSessionInput(session, "mem_in");   // 输入名称需确认
    
    // 输出张量名称列表（需与模型实际输出顺序一致）
    const std::vector<std::string> output_names = {
        "angle", "shape", "svd_pt", "score", "mem_out"
    };

    // 处理每个样本
    cout << "going to process feat and mem file" << endl;
    for (int sample_id = 0; sample_id < 1; ++sample_id) {
        char base_name[9];
        snprintf(base_name, sizeof(base_name), "%08d", sample_id);
        
        // 构建文件路径
        std::string feat_path = input_dir + base_name + "_feat.raw";
        std::string mem_path = input_dir + base_name + "_mem_in.raw";
        std::string sample_output_dir = output_dir + base_name + "/";

        //创建输出目录ƒ
        struct stat st;
        if (stat(sample_output_dir.c_str(), &st) != 0) {
            // 目录不存在，尝试创建
            cout << "mkdir start: " << sample_output_dir << endl;
            int ret = mkdir(sample_output_dir.c_str(), 0755);
            if (ret != 0) {
                cout << "mkdir " << sample_output_dir " failed: " << strerror(errno) << endl;
            }
            CHECK(0 == ret);
        }

        try {
            // 读取feat_data输入数据
            std::ifstream feat_file(feat_path, std::ios::binary | std::ios::ate);
            size_t expect_size = std::accumulate(feat_shape.begin(), feat_shape.end(), 1, std::multiplies<>());
            if (!feat_file.is_open()) {
                throw std::runtime_error("无法打开文件: " + feat_path);
            }
            
            size_t file_size = feat_file.tellg();
            if (file_size != expect_size * sizeof(float)) {
                throw std::runtime_error("文件大小不匹配: " + feat_path);
            }
            
            feat_file.seekg(0);
            std::vector<float> feat_data(expect_size);
            feat_file.read(reinterpret_cast<char*>(feat_data.data()), file_size);

            //读取mem_data输入数据
            std::ifstream mem_file(mem_path, std::ios::binary | std::ios::ate);
            expect_size = std::accumulate(mem_in_shape.begin(), mem_in_shape.end(), 1, std::multiplies<>());
            if (!mem_file.is_open()) {
                throw std::runtime_error("无法打开文件: " + mem_path);
            }
            
            file_size = mem_file.tellg();
            if (file_size != expect_size * sizeof(float)) {
                throw std::runtime_error("文件大小不匹配: " + mem_path);
            }
            
            mem_file.seekg(0);
            std::vector<float> mem_data(expect_size);
            mem_file.read(reinterpret_cast<char*>(mem_data.data()), file_size);

            // 拷贝数据到输入张量
            auto feat_tensor = MNN::Tensor::create<float>(feat_shape, feat_data.data(), MNN::Tensor::CAFFE);
            feat_input->copyFromHostTensor(feat_tensor);
            
            auto mem_tensor = MNN::Tensor::create<float>(mem_in_shape, mem_data.data(), MNN::Tensor::CAFFE);
            mem_input->copyFromHostTensor(mem_tensor);

            // 执行推理
            interpreter->runSession(session);

            // 处理输出
            for (const auto& name : output_names) {
                auto output_tensor = interpreter->getSessionOutput(session, name.c_str());
                MNN::Tensor host_tensor(output_tensor, output_tensor->getDimensionType());
                output_tensor->copyToHostTensor(&host_tensor);
                
                // 保存结果
                std::string output_path = sample_output_dir + name + ".txt";
                size_t count = host_tensor.elementSize();
                std::ofstream file(output_path);
                if (!file.is_open()) {
                    throw std::runtime_error("无法创建文件: " + output_path);
                }
                
                file.precision(8);
                file << std::fixed;

                // 每个数值单独一行
                for (size_t i = 0; i < count; ++i) {
                    file << host_tensor.host<float>()[i] << "\n";  // 用换行符代替逗号
                }
            }
            
            std::cout << "处理完成: " << base_name << std::endl;
        } 
        catch (const std::exception& e) {
            std::cerr << "处理失败 " << base_name << ": " << e.what() << std::endl;
        }
    }

    return 0;
}