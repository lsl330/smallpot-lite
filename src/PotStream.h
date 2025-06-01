﻿#pragma once

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
}

#include "Engine.h"
#include "PotBase.h"
#include <algorithm>
#include <map>
#include <mutex>

enum PotMediaType
{
    MEDIA_TYPE_VIDEO = AVMEDIA_TYPE_VIDEO,
    MEDIA_TYPE_AUDIO = AVMEDIA_TYPE_AUDIO,
    MEDIA_TYPE_SUBTITLE = AVMEDIA_TYPE_SUBTITLE,
};

/*
Context - 未解码数据
Packet - 读取的一个未解的包
Frame - 解得的一帧数据
Content - 转换而得的可以直接显示或播放的数据，包含时间，信息（通常为总字节），和指向数据区的指针
*/

struct FrameContent
{
    int time;
    int64_t info;
    void* data;
};

class PotStream : public PotBase
{
public:
    std::map<int, FrameContent> data_map_;    //解压出的内容的map，key是时间
    //std::vector<int> stream_index_vector_;

    PotStream();
    virtual ~PotStream();

protected:
    PotMediaType type_;
    AVFormatContext* format_ctx_ = nullptr;
    AVStream* stream_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
    const AVCodec* codec_ = nullptr;

    AVFrame* frame_ = nullptr;
    AVPacket packet_;

    bool need_read_packet_ = true;
    int stream_index_ = -1;
    double time_per_frame_ = 0, time_base_packet_ = 0;
    int max_size_ = 1;    //为0时仅预解一帧, 理论效果与=1相同, 但不使用map和附加缓冲区
    std::string filename_;

    int ticks_shown_ = -1;    //最近展示的ticks

    int time_dts_ = 0, time_pts_ = 0, time_shown_ = 0;    //解压时间，应展示时间，最近已经展示的时间
    int time_other_ = 0;
    int start_time_ = 0;
    int total_time_ = 0;

    bool pause_ = false;
    int pause_time_ = 0;
    bool key_frame_ = false;
    int data_length_ = 0;
    bool stopping_ = false;    //表示放弃继续解压这个流
    int decode_frame_count_ = 1;

private:
    bool decoded_ = false, skip_ = false, ended_ = false, seeking_ = false;
    int seek_record_ = 0;    //上次seek的记录
    virtual int avcodec_decode_packet(AVCodecContext* ctx, int* n, AVPacket* packet);

private:
    virtual FrameContent convertFrameToContent() { return { 0, 0, nullptr }; }
    int dropContent();
    void setMap(int key, FrameContent f);
    virtual void freeContent(void* p) {};
    void clearMap();
    bool needDecode();
    virtual bool needDecode2() { return true; };
    virtual int decodeNextPacketToFrame(bool decode, bool til_got);

protected:
    void setDecoded(bool b);
    bool haveDecoded();
    void dropAllDecoded();
    FrameContent getCurrentContent();

public:
    virtual int openFile(const std::string& filename);
    int tryDecodeFrame(bool reset = false);
    void dropDecoded();
    int getTotalTime();

    void setSkip(bool b) { skip_ = b; }
    void resetTimeBegin() { ticks_shown_ = -1; }

    int seek(int time, int direct = 1, int reset = 0);
    void setFrameTime();
    int getTime();
    int setAnotherTime(int time);
    int skipFrame(int time);

    void getSize(int& w, int& h);
    virtual int getWidth() { return exist() ? codec_ctx_->width : 0; }
    virtual int getHeight() { return exist() ? codec_ctx_->height : 0; }
    int getTimedts() { return time_dts_ > 0 ? time_dts_ : time_pts_; }
    int getTimeShown() { return time_shown_; }
    virtual bool exist() { return stream_index_ >= 0; }
    void resetTimeAxis(int time);
    bool isPause() { return pause_; }
    bool isKeyFrame() { return key_frame_; }
    virtual void setPause(bool pause);
    void resetDecoderState() { avcodec_flush_buffers(codec_ctx_); }
    double getRotation();
    void getRatio(int& x, int& y);
    int getRatioX() { return exist() ? (std::max)(stream_->sample_aspect_ratio.num, 1) : 1; }
    int getRatioY() { return exist() ? (std::max)(stream_->sample_aspect_ratio.den, 1) : 1; }
    bool isStopping() { return stopping_; }

    PotMediaType getType() { return type_; }
    void setStreamIndex(int i) { stream_index_ = i; }
    int getStreamIndex() { return stream_index_; }
    void setFormatCtx(AVFormatContext* format_ctx) { format_ctx_ = format_ctx; }
    void initFromCodecpar(AVCodecParameters* codecpar) {
        const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
        if (!codec) {
            SDL_Log("Error: Codec not found for video stream. Codec ID: %d", codecpar->codec_id);
            return;
        }

        codec_ctx_ = avcodec_alloc_context3(codec);
        if (!codec_ctx_) {
            SDL_Log("Error: Failed to allocate codec context for video stream.");
            return;
        }

        int ret = avcodec_parameters_to_context(codec_ctx_, codecpar);
        if (ret < 0) {
            char err_buf[1024];
            av_strerror(ret, err_buf, sizeof(err_buf));
            SDL_Log("Error: Failed to copy codec parameters. Error: %s", err_buf);
            avcodec_free_context(&codec_ctx_); // 释放已分配的上下文
            return;
        }

        ret = avcodec_open2(codec_ctx_, codec, nullptr);
        if (ret < 0) {
            char err_buf[1024];
            av_strerror(ret, err_buf, sizeof(err_buf));
            SDL_Log("Error: Failed to open video codec. Error: %s", err_buf);
            avcodec_free_context(&codec_ctx_); // 释放已分配的上下文
            return;
        }
        //this->stream_ = format_ctx_->streams[stream_index_]; // 需要确保此处正确关联
    }
};
