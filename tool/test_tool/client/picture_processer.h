#ifndef __PICTURE_PROCESEESER_H__
#define __PICTURE_PROCESEESER_H__

#include "common.h"
#include "grpc_client.h"

// 图片集测试 业务流程实现
class PictureProcesser {
   public:
    PictureProcesser(std::shared_ptr<AiClientImpl>& grpc_client) { m_grpc_client = grpc_client; }
    ~PictureProcesser() = default;

    void BindBenchmarkDatas(std::shared_ptr<ToolsDataSet>& benchmark_datas, TestConfig& tconfig);
    void Start();
    void WaitAndStop();
    int GetReportResult(ToolsResults& result);

   private:
    // 获取下一组测试图片
    int GetNextPicData(cv::Mat& src_img1, cv::Mat& src_img2, HeadPoseData& headpose, uint64_t& timestamp,
                       std::string& token);
    // 循环发送图片
    void SendPic();
    // 查询server端未完成的数据信息
    void QueryResultState(uint64_t& wait_near_frame_id, uint64_t& wait_frame_num);
    // 循环接受图片
    void RecvResult();

   public:
    // 结果缓存队列
    std::mutex m_result_lock;
    std::set<uint64_t> m_result_frameids;
    std::map<uint64_t, ToolsResults> m_results;
    // 结果缓存队列
    std::mutex m_result_lock_v2;
    std::list<ToolsResults> m_results_v2;
    bool test_finish = false;
    bool abort_finish = false;

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
    uint32_t max_cache_fn = 0;
    // 测试图片集
    bool is_lmdb = false;
    std::shared_ptr<ToolsDataSet> m_benchmark_datas;
    uint32_t count = 0;
};

#endif