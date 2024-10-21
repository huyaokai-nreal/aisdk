#ifndef __VIDEO_PROCESEESER_H__
#define __VIDEO_PROCESEESER_H__

#include "common.h"
#include "grpc_client.h"

// 视频集测试 业务流程实现
class VideoProcesser {
   public:
    VideoProcesser(std::shared_ptr<AiClientImpl>& grpc_client);
    ~VideoProcesser();

    void BindBenchmarkDatas(std::string& left_camera_stream_path, std::string& right_camera_stream_path,
                            int32_t max_used_frame_num);
    void Start();
    void WaitAndStop();

   private:
    // 循环发送视频解码帧
    void SendFrame();
    // 循环接受图片
    void RecvResult();

   public:
    // 结果缓存队列
    std::mutex m_result_lock;
    std::set<uint64_t> m_result_frameids;
    std::map<uint64_t, ToolsResults> m_results;

   private:
    // 收发线程控制
    // 重要：目前设计是按(不重复，递增, >0的)frameid来实现顺序收发和结果匹配的。
    uint32_t send_count = 0;
    uint32_t recv_count = 0;
    bool send_exit = false;
    bool recv_exit = false;
    std::thread m_send_th;
    std::thread m_recv_th;

   private:
    // grpc client 接口
    std::shared_ptr<AiClientImpl> m_grpc_client;
    int32_t input_width;
    int32_t input_height;
    // 测试视频集
    std::string l_path;
    std::string r_path;
    uint32_t count = 0;
    void* m_l_decoder;
    void* m_r_decoder;
};

#endif
