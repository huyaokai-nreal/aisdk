#include "picture_processer.h"

void PictureProcesser::BindBenchmarkDatas(std::shared_ptr<ToolsDataSet>& benchmark_datas, TestConfig& tconfig) {
    m_benchmark_datas = benchmark_datas;
    is_lmdb = (tconfig.stream_type == PICTURE_LMDB_STREAM || tconfig.stream_type == PICTURE_GT_LMDB_STREAM);
    if (is_lmdb) {
        count = m_benchmark_datas->lmdb_meta.lmdbdata_list.size();
    } else {
        count = m_benchmark_datas->pic_datas.size();
    }

    if (tconfig.max_used_frame_num > 0 && tconfig.max_used_frame_num < count) {
        count = tconfig.max_used_frame_num;
    }
    max_cache_fn = tconfig.max_cache_frame_num;
    input_width = tconfig.input_width;
    input_height = tconfig.input_height;
}

void PictureProcesser::Start() {
    send_exit = false;
    recv_exit = false;
    m_send_th = std::move(std::thread(&PictureProcesser::SendPic, this));
    m_recv_th = std::move(std::thread(&PictureProcesser::RecvResult, this));
}

void PictureProcesser::WaitAndStop() {
    if (m_send_th.joinable()) {
        m_send_th.join();
    }

    if (m_recv_th.joinable()) {
        m_recv_th.join();
    }
}

int PictureProcesser::GetNextPicData(cv::Mat& src_img1, cv::Mat& src_img2, HeadPoseData& headpose, uint64_t& timestamp,
                                     std::string& token) {
    if (false == is_lmdb) {
        // 按时间戳排序的前n张作为测试
        static std::map<uint64_t, PictureData>::iterator iter = m_benchmark_datas->pic_datas.begin();
        if (iter != m_benchmark_datas->pic_datas.end()) {
            std::string l_file = iter->second.l_pic;
            std::string r_file = iter->second.r_pic;
            src_img1 = cv::imread(l_file.c_str(), cv::IMREAD_GRAYSCALE);
            src_img2 = cv::imread(r_file.c_str(), cv::IMREAD_GRAYSCALE);
            timestamp = iter->second.timestamp;
            token = iter->second.token;
            if (iter->second.is_match_pose) {
                auto iter2 = m_benchmark_datas->pose_datas.find(iter->first);
                headpose = iter2->second;
            }
            iter++;
        } else {
            return 1;
        }
    } else {
        static uint64_t db_idx = 0;
        if (db_idx < m_benchmark_datas->lmdb_meta.lmdbdata_list.size()) {
            for (int i = 0; i < 2; i++) {
                std::string str_id;
                if (0 == i) {
                    str_id = m_benchmark_datas->lmdb_meta.lmdbdata_list[db_idx].l_key;
                } else {
                    str_id = m_benchmark_datas->lmdb_meta.lmdbdata_list[db_idx].r_key;
                }

                lmdb::val lmdb_key(str_id);
                lmdb::val lmdb_value;

                bool db_get_ok =
                    m_benchmark_datas->lmdb_datas.dbi.get(m_benchmark_datas->lmdb_datas.rtxn, lmdb_key, lmdb_value);
                if (db_get_ok) {
                    cv::_InputArray data((const uchar*)lmdb_value.data(), lmdb_value.size());
                    if (0 == i) {
                        src_img1 = cv::imdecode(data, cv::IMREAD_GRAYSCALE);
                        token = str_id;
                    } else {
                        src_img2 = cv::imdecode(data, cv::IMREAD_GRAYSCALE);
                    }
                } else {
                    std::cout << "lmdb dbi.get " << str_id << " error!" << std::endl;
                }
            }
            token = m_benchmark_datas->lmdb_meta.lmdbdata_list[db_idx].token;
            timestamp = m_benchmark_datas->lmdb_meta.lmdbdata_list[db_idx].timestamp;
            if (m_benchmark_datas->lmdb_meta.lmdbdata_list[db_idx].is_match_pose) {
                auto iter2 = m_benchmark_datas->pose_datas.find(timestamp);
                headpose = iter2->second;
            }
            db_idx++;
        } else {
            return 1;
        }
    }

    if (src_img1.empty() || src_img2.empty()) {
        return -3;
    }

    if (false == m_grpc_client->server_auto_cv) {
        if (src_img1.cols != input_width || src_img1.rows != input_height) {
            printf("[warning] test input pic size != sdk input size \n");
            printf("Client input WH=%d,%d type=%d -> sdk input WH=%d,%d\n", src_img1.cols, src_img1.rows,
                   src_img1.type(), input_width, input_height);
            cv::Mat src_img11;
            cv::resize(src_img1, src_img11, cv::Size(input_width, input_height));
            src_img1 = std::move(src_img11);
        }

        if (src_img2.cols != input_width || src_img2.rows != input_height) {
            printf("[warning] test input pic size != sdk input size \n");
            printf("Client input WH=%d,%d type=%d -> sdk input WH=%d,%d\n", src_img1.cols, src_img1.rows,
                   src_img1.type(), input_width, input_height);
            cv::Mat src_img22;
            cv::resize(src_img2, src_img22, cv::Size(input_width, input_height));
            src_img2 = std::move(src_img22);
        }
    }

    return 0;
}

void PictureProcesser::SendPic() {
    while (1) {
        // 读图片
        cv::Mat src_img1;
        cv::Mat src_img2;
        HeadPoseData hd;
        uint64_t timestamp = 0;
        std::string token;
        bool test_over = false;
        if (send_count < count) {
            int gret = GetNextPicData(src_img1, src_img2, hd, timestamp, token);
            if (gret < 0) {
                send_count++;
                std::cout << "[SendPic] GetNextPicData error " << std::endl;
                continue;
            } else if (gret > 0) {
                test_over = true;
            }
        } else {
            test_over = true;
        }

        // 准备推理数据
        NrealAiTool::PipelineInferenceRequest request;
        request.set_session_id(m_grpc_client->session_id);
        if (false == test_over) {
            request.set_is_eof(false);
            request.set_frame_id(send_count + 1);
            request.set_width(src_img1.cols);
            request.set_height(src_img1.rows);
            request.set_data_format(NrealAiTool::DataType::GRAY_8UC1);
            request.set_nano_time(timestamp);
            // 目前仅先实现单通道的数据copy
            if (CV_8UC1 == src_img1.type()) {
                char* pData = (char*)src_img1.data;
                auto lens = src_img1.cols * src_img1.rows;
                request.mutable_left_camera_frame()->resize(lens);
                memcpy((char*)request.mutable_left_camera_frame()->data(), pData, lens);
            }
            if (CV_8UC1 == src_img2.type()) {
                char* pData = (char*)src_img2.data;
                auto lens = src_img2.cols * src_img2.rows;
                request.mutable_right_camera_frame()->resize(lens);
                memcpy((char*)request.mutable_right_camera_frame()->data(), pData, lens);
            }
            for (uint32_t i = 0; i < 7; i++) {
                request.mutable_head_pose()->add_transform(hd.headpose[i]);
            }
            request.set_token(token);
        } else if (test_over) {
            request.set_is_eof(true);
        }

        // 异常处理 或者 忙的时候等待
        bool abort = false;
        int ret = 0;
        while (1) {
            ret = m_grpc_client->SendFrame(request);
            if (-1 == ret) {
                std::cout << "[SendPic error] grpc disconnect " << std::endl;
                abort = true;
            } else if (-100 == ret) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            break;
        }

        if (abort || test_over) {
            break;
        }

        // 发送成功记录，等待收取结果
        send_count++;
        std::cout << "rate of SendFrame : " << send_count << "/" << count << std::endl;
    }

    recv_exit = true;
}

void PictureProcesser::QueryResultState(uint64_t& wait_near_frame_id, uint64_t& wait_frame_num) {
    std::lock_guard<std::mutex> guard(m_result_lock);
    wait_frame_num = m_result_frameids.size();

    if (wait_frame_num > 0) {
        auto iter = m_result_frameids.begin();
        wait_near_frame_id = *iter;
    } else {
        wait_near_frame_id = uint64_t(-1);
    }
}

void PictureProcesser::RecvResult() {
    bool abort = false;
    while (1) {
        uint64_t frame_id = 0;
        // 正常需要取结果
        if (uint64_t(-1) != frame_id) {
            NrealAiTool::GetPipelineResultReply response;
            int ret = m_grpc_client->RecvPipelineResult(frame_id, &response);
            if (0 == ret) {
                std::map<std::string, std::string> standard_config_params(response.result_info().begin(),
                                                                          response.result_info().end());
                {
                    ToolsResults res;
                    res.frame_id = response.frame_id();
                    res.token = response.token();
                    res.info_type = (int)response.result_type();
                    res.m_info = std::move(standard_config_params);
                    std::lock_guard<std::mutex> guard(m_result_lock_v2);
                    m_results_v2.emplace_back(std::move(res));
                }

                recv_count++;
                std::cout << "rate of RecvResult : " << recv_count << "/" << count << std::endl;
            } else if (-1 == ret) {
                std::cout << "[RecvResult error] grpc disconnect " << std::endl;
                abort_finish = true;
                abort = true;
            } else if (-100 == ret) {
                test_finish = true;
                std::cout << "[RecvResult ok] test over " << std::endl;
                abort = true;
            } else if (-2 == ret) {
                // 暂无结果
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (abort) {
            break;
        }
    }
}

int PictureProcesser::GetReportResult(ToolsResults& result) {
    std::lock_guard<std::mutex> guard(m_result_lock_v2);
    if (m_results_v2.size()) {
        auto& tmp = m_results_v2.front();
        result = std::move(tmp);
        m_results_v2.pop_front();
        return 0;
    }

    if (abort_finish) {
        return -99;
    }

    if (test_finish) {
        return -100;
    }
    return -1;
}