#include "report_result.h"

#include <fstream>

#include "aisdk/base/file.h"
#include "fmt/format.h"
ReportResult::ReportResult(std::string& out_dir, std::string& result_process) {
    aisdk::base::CreateDir(out_dir);
    m_out_dir = out_dir;
    // auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    // std::stringstream ss;
    // ss << std::put_time(std::localtime(&t), "%Y-%m-%d_%H-%M-%S");
    // std::string str = ss.str();
    // m_out_dir = out_dir + "/" + str;
    // Mkdir(m_out_dir);

    m_result_process = result_process;
}

void ReportResult::show(std::map<uint64_t, ToolsResults>& m_results) {
    std::cout << std::endl << "[ReportResult] START /********************************/ " << std::endl;
    // 打印测试结果
    if (SHOW_RESULT_STDOUT == m_result_process) {
        PrintfPerFrame(m_results);
        // 存储成json文件
    } else if (SHOW_RESULT_JSON == m_result_process) {
        SaveJsonPerFrame(m_results);
        std::cout << std::endl << "[ReportResult] Save Dir: " << m_out_dir << std::endl;
    }
    std::cout << std::endl << "[ReportResult] END /********************************/ " << std::endl;
}

void ReportResult::show(ToolsResults& m_results) {
    static uint64_t sequence_id1 = 0;
    static uint64_t sequence_id2 = 0;
    // 打印测试结果
    if (SHOW_RESULT_STDOUT == m_result_process) {
        // 存储成json文件
    } else if (SHOW_RESULT_JSON == m_result_process) {
        std::string output_file;
        std::string sequence;
        if (1 == m_results.info_type) {
            sequence = fmt::format("{:06}", sequence_id1++);
            output_file = m_out_dir + "/profiling" + sequence + ".json";
        } else if (2 == m_results.info_type) {
            sequence = fmt::format("{:06}", sequence_id2++);
            output_file = m_out_dir + "/result" + sequence + ".json";
        }

        Json::Value json_result;
        if (1 == m_results.info_type) {
            json_result["test_sequence"] = sequence;
            json_result["frame_id"] = m_results.frame_id;
            json_result["token"] = m_results.token;
        } else {
            json_result["test_sequence"] = sequence;
        }
        for (auto item = m_results.m_info.begin(); item != m_results.m_info.end(); ++item) {
            Json::Value tmp;
            Json::Reader reader;
            reader.parse(item->second, tmp);
            json_result[item->first] = tmp;
        }

        std::ofstream out(output_file);
        // Json::FastWriter fwriter;
        Json::StyledWriter fwriter;
        if (out.is_open()) {
            out << fwriter.write(json_result);
            out.close();
        }
    }
}

void ReportResult::SaveJsonPerFrame(std::map<uint64_t, ToolsResults>& m_results) {
    for (auto it = m_results.begin(); it != m_results.end(); ++it) {
        std::string output_file = m_out_dir + "/frame" + std::to_string(it->first) + ".json";
        Json::Value json_result;
        json_result["frame_id"] = it->second.frame_id;
        json_result["token"] = it->second.token;
        for (auto item = it->second.m_info.begin(); item != it->second.m_info.end(); ++item) {
            Json::Value tmp(item->second);
            json_result[item->first] = tmp;
        }

        std::ofstream out(output_file);
        Json::FastWriter fwriter;
        if (out.is_open()) {
            out << fwriter.write(json_result);
            out.close();
        }
    }
}

void ReportResult::PrintfPerFrame(std::map<uint64_t, ToolsResults>& m_results) {
    for (auto it = m_results.begin(); it != m_results.end(); ++it) {
        std::cout << "[PrintfPerFrame] frame_id= " << it->first << std::endl;
        std::cout << "[PrintfPerFrame] token= " << it->second.token << std::endl;
        for (auto item = it->second.m_info.begin(); item != it->second.m_info.end(); ++item) {
            std::cout << "[PrintfPerFrame meta] key=" << item->first << " value=" << item->second << std::endl;
        }
    }
}
