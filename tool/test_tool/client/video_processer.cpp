#include "video_processer.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/adler32.h"
#include "libavutil/imgutils.h"
#include "libavutil/timestamp.h"
#include "libswscale/swscale.h"
}

class FfmpegDecoder {
   public:
    FfmpegDecoder(std::string& stream_file, AVPixelFormat format, int width, int height) {
        m_stream_file = stream_file;
        m_user_format = format;
        m_user_width = width;
        m_user_height = height;

        frameCout = 0;
        if (0 != OpenDecoder()) {
            CloseDecoder();
        }
    }

    ~FfmpegDecoder() { CloseDecoder(); }

    int ReadFrame() {
        int ret = -1;
        int gotPicture = 0;
        if (formatContext) {
            while (1) {
                av_init_packet(&packet);
                ret = av_read_frame(formatContext, &packet);
                if (ret >= 0) {
                    if (packet.stream_index == videoStreamIndex) {
                        gotPicture = 0;
                        if (packet.pts == AV_NOPTS_VALUE) packet.pts = packet.dts = frameCout;

                        ret = avcodec_decode_video2(codecContext, frame, &gotPicture, &packet);
                        if (ret < 0) {
                            // 解码异常
                            ret = -3;
                        } else {
                            if (gotPicture) {
                                // 成功解码出图片
                                // 后续利用swscale或者filter做图像缩放 或者 拷贝到其他内存
                                frameCout++;
                                ret = 0;
                            } else {
                                // 解码出非图片数据,需要跳过
                                ret = 2;
                            }
                        }
                    } else {
                        // 读取到其他流的信息,需要跳过
                        ret = 1;
                    }
                    av_packet_unref(&packet);
                } else {
                    // 文件读到末尾
                    ret = -2;
                }

                if (ret <= 0) {
                    break;
                }
            }
        }
        // 文件打开失败
        return ret;
    }

    int ReadFramePlus(cv::Mat& dst_frame, uint64_t* nano_time) {
        int ret = 0;
        ret = ReadFrame();
        if (0 == ret) {
            // std::cout << "-------frameid:" << frameCout << std::endl;
            // std::cout << "frame->data[0]:" << (void*)frame->data[0] << std::endl;
            // std::cout << "frame->data[1]:" << (void*)frame->data[1] << std::endl;
            // std::cout << "frame->data[2]:" << (void*)frame->data[2] << std::endl;

            // std::cout << "frame->linesize[0]:" << frame->linesize[0] << std::endl;
            // std::cout << "frame->linesize[1]:" << frame->linesize[1] << std::endl;
            // std::cout << "frame->linesize[2]:" << frame->linesize[2] << std::endl;

            // std::cout << "frame->width:" << frame->width << std::endl;
            // std::cout << "frame->height:" << frame->height << std::endl;
            // std::cout << "frame->pts:" << frame->pts << std::endl;
            // std::cout << "packet->pts:" << packet.pts << std::endl;
            // std::cout << "packet->pts:" << packet.dts << std::endl;
            // std::cout << "frame->key_frame:" << frame->key_frame << std::endl;
            // std::cout << "frame->format:" << AVPixelFormat(frame->format) << std::endl;
            // std::cout << "frame->channels:" << frame->channels << std::endl;
            // std::cout << "codecContext->pix_fmt: " << codecContext->pix_fmt << std::endl;
            // std::cout << "codecContext->width: " << codecContext->width << std::endl;
            // std::cout << "codecContext->height: " << codecContext->height << std::endl;

            // std::cout << "m_user_height: " << m_user_height<< std::endl;
            // std::cout << "m_user_width: " << m_user_width << std::endl;
            // std::cout << "m_user_format: " << m_user_format << std::endl;
            int type = CV_8UC1;
            if (m_user_format == AV_PIX_FMT_RGB24 || m_user_format == AV_PIX_FMT_BGR24) {
                type = CV_8UC3;
            }

            if (codecContext->pix_fmt != m_user_format || codecContext->width != m_user_width ||
                codecContext->height != m_user_height) {
                int oh = sws_scale(sws_ctx, (const uint8_t* const*)frame->data, frame->linesize, 0,
                                   codecContext->height, convertframe.data, convertframe.linesize);
                // std::cout << "oh: " << oh << std::endl;
                dst_frame = std::move(cv::Mat(m_user_height, m_user_width, type, byte_buffer));
                *nano_time = frame->pts;
                ret = 0;
            } else {
                int number_of_written_bytes =
                    av_image_copy_to_buffer(byte_buffer, byte_buffer_size, (const uint8_t* const*)frame->data,
                                            (const int*)frame->linesize, m_user_format, m_user_width, m_user_height, 1);
                if (number_of_written_bytes < 0) {
                    // 内存操作异常
                    ret = -4;
                } else {
                    dst_frame = std::move(cv::Mat(m_user_height, m_user_width, type, byte_buffer));
                    *nano_time = frame->pts;
                    ret = 0;
                }
            }
        }

        return ret;
    }

   private:
    int OpenDecoder() {
        int ret = 0;
        ret = avformat_open_input(&formatContext, m_stream_file.c_str(), NULL, NULL);
        if (0 != ret) {
            std::cout << " avformat_open_input is error. ret=%d " << ret << std::endl;
            return -1;
        }

        ret = avformat_find_stream_info(formatContext, NULL);
        if (ret < 0) {
            std::cout << " avformat_find_stream_info is error. ret=%d " << ret << std::endl;
            return -2;
        }

        videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (videoStreamIndex < 0) {
            std::cout << " av_find_best_stream videoStreamIndex is error" << std::endl;
            return -3;
        }

        origin_par = formatContext->streams[videoStreamIndex]->codecpar;

        codec = avcodec_find_decoder(origin_par->codec_id);
        if (!codec) {
            std::cout << " avcodec_find_decoder codec is error" << std::endl;
            return -4;
        }

        codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            std::cout << " avcodec_alloc_context3 codec is error" << std::endl;
            return -5;
        }

        ret = avcodec_parameters_to_context(codecContext, origin_par);
        if (ret) {
            std::cout << " avcodec_parameters_to_context codec is error" << std::endl;
            return -6;
        }

        ret = avcodec_open2(codecContext, codec, NULL);
        if (ret < 0) {
            std::cout << " avcodec_open2 is error. ret=%d " << ret << std::endl;
            return -7;
        }

        std::cout << "iformat->name:" << formatContext->iformat->name << std::endl;
        std::cout << "duration:" << formatContext->duration / 1000000 << std::endl;
        std::cout << "WxH:" << codecContext->width << "," << codecContext->height << std::endl;
        std::cout << "codec->name:" << codec->name << std::endl;

        frame = av_frame_alloc();
        if (!frame) {
            std::cout << " av_frame_alloc is error. " << std::endl;
            return -8;
        }

        byte_buffer_size = av_image_get_buffer_size(m_user_format, m_user_width, m_user_height, 1);
        byte_buffer = (uint8_t*)av_malloc(byte_buffer_size);
        if (!byte_buffer) {
            std::cout << " av_malloc is error. " << std::endl;
            return -9;
        }

        // 解码一般解码出的是 codecContext->pix_fmt == AV_PIX_FMT_YUV420P
        if (codecContext->pix_fmt != m_user_format || codecContext->width != m_user_width ||
            codecContext->height != m_user_height) {
            sws_ctx = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt, m_user_width,
                                     m_user_height, m_user_format, SWS_BILINEAR, NULL, NULL, NULL);
            if (!sws_ctx) {
                std::cout << " sws_getContext is error. " << std::endl;
                return -10;
            }
            avpicture_fill(&convertframe, byte_buffer, m_user_format, m_user_width, m_user_height);
        }

        std::cout << "open ok :" << m_stream_file << std::endl;
        return 0;
    }

    void CloseDecoder() {
        if (formatContext) {
            if (byte_buffer) {
                av_freep(&byte_buffer);
            }

            if (sws_ctx) {
                sws_freeContext(sws_ctx);
            }

            if (frame) {
                av_frame_free(&frame);
            }

            if (codecContext) {
                avcodec_close(codecContext);
                avcodec_free_context(&codecContext);
            }

            avformat_close_input(&formatContext);
        }
    }

   private:
    std::string m_stream_file;
    AVPixelFormat m_user_format;
    int m_user_width = 0;
    int m_user_height = 0;
    int frameCout = 0;
    // 解码器相关
    AVFormatContext* formatContext = nullptr;
    AVCodecParameters* origin_par = NULL;
    int videoStreamIndex;
    AVCodecContext* codecContext = nullptr;
    AVCodec* codec = nullptr;
    // 解码帧相关
    AVPacket packet;
    AVFrame* frame = nullptr;
    // 图片后处理相关
    int byte_buffer_size = 0;
    uint8_t* byte_buffer = nullptr;
    AVPicture convertframe;
    struct SwsContext* sws_ctx = nullptr;
};

VideoProcesser::VideoProcesser(std::shared_ptr<AiClientImpl>& grpc_client) { m_grpc_client = grpc_client; }

VideoProcesser::~VideoProcesser() {
    if (m_l_decoder) {
        delete (FfmpegDecoder*)m_l_decoder;
        m_l_decoder = nullptr;
    }

    if (m_r_decoder) {
        delete (FfmpegDecoder*)m_r_decoder;
        m_r_decoder = nullptr;
    }
}

void VideoProcesser::BindBenchmarkDatas(std::string& left_camera_stream_path, std::string& right_camera_stream_path,
                                        int32_t max_used_frame_num) {
    l_path = left_camera_stream_path;
    r_path = right_camera_stream_path;
    count = (max_used_frame_num > 0) ? max_used_frame_num : 0;

    AVPixelFormat format = AV_PIX_FMT_GRAY8;
    // if (m_grpc_client->input_datatype == NrealAiTool::DataType::GRAY_8UC1) {
    //     format = AV_PIX_FMT_GRAY8;
    // } else if (m_grpc_client->input_datatype == NrealAiTool::DataType::RGB_8UC3) {
    //     format = AV_PIX_FMT_RGB24;
    // } else if (m_grpc_client->input_datatype == NrealAiTool::DataType::BGR_8UC3) {
    //     format = AV_PIX_FMT_BGR24;
    // }

    m_l_decoder = (void*)new FfmpegDecoder(left_camera_stream_path, format, input_width, input_height);
    m_r_decoder = (void*)new FfmpegDecoder(right_camera_stream_path, format, input_width, input_height);
}

void VideoProcesser::Start() {
    send_exit = false;
    recv_exit = false;
    m_send_th = std::move(std::thread(&VideoProcesser::SendFrame, this));
    m_recv_th = std::move(std::thread(&VideoProcesser::RecvResult, this));
}

void VideoProcesser::WaitAndStop() {
    if (m_send_th.joinable()) {
        m_send_th.join();
    }

    if (m_recv_th.joinable()) {
        m_recv_th.join();
    }
}

void VideoProcesser::SendFrame() {
    while (1) {
        uint64_t nano_time;
        cv::Mat src_img1;
        cv::Mat src_img2;
        int ret1 = ((FfmpegDecoder*)m_l_decoder)->ReadFramePlus(src_img1, &nano_time);
        int ret2 = ((FfmpegDecoder*)m_r_decoder)->ReadFramePlus(src_img2, &nano_time);
        if (ret1 || ret2) {
            std::cout << "[SendFrame over] ReadFrame file over; ret=" << ret1 << "/" << ret2 << std::endl;
            break;
        }

        if (count && send_count >= count) {
            std::cout << "[SendFrame over] send_count >= count " << std::endl;
            break;
        }

        if (false == m_grpc_client->server_auto_cv) {
            if (src_img1.cols != input_width || src_img1.rows != input_height) {
                printf("w=%d h=%d type=%d \n", src_img1.cols, src_img1.rows, src_img1.type());
                cv::Mat src_img11;
                cv::resize(src_img1, src_img11, cv::Size(input_width, input_height));
                src_img1 = std::move(src_img11);
            }

            if (src_img2.cols != input_width || src_img2.rows != input_height) {
                printf("w=%d h=%d type=%d \n", src_img2.cols, src_img2.rows, src_img2.type());
                cv::Mat src_img22;
                cv::resize(src_img2, src_img22, cv::Size(input_width, input_height));
                src_img2 = std::move(src_img22);
            }

            // if(src_img1.type() != CV_8UC1) {
            //     cv::cvtColor(src,dst,CV_BGR2GRAY);
            // }
        }

        NrealAiTool::PipelineInferenceRequest request;
        request.set_session_id(m_grpc_client->session_id);
        request.set_frame_id(send_count + 1);
        request.set_width(src_img1.cols);
        request.set_height(src_img1.rows);
        request.set_is_eof(false);
        // request.set_data_format(m_grpc_client->input_datatype);
        request.set_nano_time(nano_time);
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

        bool abort = false;
        int ret = 0;
        while (1) {
            ret = m_grpc_client->SendFrame(request);
            if (-1 == ret) {
                std::cout << "[SendFrame error] grpc disconnect " << std::endl;
                abort = true;
            } else if (-100 == ret) {
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
                continue;
            }
            break;
        }

        if (abort) {
            break;
        }

        send_count++;
        if (0 == ret) {
            ToolsResults tmp;
            auto fid = request.frame_id();

            std::lock_guard<std::mutex> guard(m_result_lock);
            m_result_frameids.insert(fid);
            m_results.insert(std::make_pair(fid, tmp));
            m_results[fid].frame_id = fid;
        }
        std::cout << "rate of SendFrame : " << send_count << "/" << count << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    recv_exit = true;
}

void VideoProcesser::RecvResult() {
    bool abort = false;
    while (1) {
        uint64_t frame_id = 0;
        {
            std::lock_guard<std::mutex> guard(m_result_lock);
            if (recv_exit && m_result_frameids.size() <= 0) {
                break;
            }
            auto iter = m_result_frameids.begin();
            if (iter != m_result_frameids.end()) {
                frame_id = *iter;
            } else {
                frame_id = uint64_t(-1);
            }
        }

        if (uint64_t(-1) != frame_id) {
            NrealAiTool::GetPipelineResultReply response;
            int ret = m_grpc_client->RecvPipelineResult(frame_id, &response);
            if (0 == ret) {
                std::map<std::string, std::string> standard_config_params(response.result_info().begin(),
                                                                          response.result_info().end());
                {
                    std::lock_guard<std::mutex> guard(m_result_lock);
                    m_results[frame_id].m_info = std::move(standard_config_params);
                    m_result_frameids.erase(frame_id);
                }

                recv_count++;
                std::cout << "rate of RecvResult : " << recv_count << "/" << count << std::endl;
            } else if (-1 == ret) {
                std::cout << "[RecvResult error] grpc disconnect " << std::endl;
                abort = true;
            }
        }

        if (abort) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}