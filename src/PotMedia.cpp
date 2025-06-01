#include "PotMedia.h"
#include "filefunc.h"

PotMedia::PotMedia()
{
    avformat_network_init();
    stream_video_ = &blank_video_;
    stream_audio_ = &blank_audio_;
    format_ctx_video_ = avformat_alloc_context();
    format_ctx_audio_ = avformat_alloc_context();
    format_ctx_subtitle_ = avformat_alloc_context();
}

PotMedia::~PotMedia()
{
    for (auto st : streams_)
    {
        delete st;
    }
    avformat_close_input(&format_ctx_video_);
    avformat_close_input(&format_ctx_audio_);
    avformat_close_input(&format_ctx_subtitle_);
}

int PotMedia::decodeFrame()
{
    bool need_reset = seeking_;
    //int se= engine_->getTicks();
    stream_video_->tryDecodeFrame(need_reset);
    if (stream_video_->isStopping())
    {
        return 0;
    }
    stream_audio_->tryDecodeFrame(need_reset);
    //stream_subtitle_->tryDecodeFrame(need_reset);
    //int m = _audioStream->getTimedts();
    //int n = _videoStream->getTimedts();

    //同步
    if (seeking_)
    {
        seeking_ = false;
        //seek之后，音频可能落后，需要追赶音频
        if (stream_video_->exist() && stream_audio_->exist())
        {
            //一定时间以上才跳帧
            //查看延迟情况
            int v_dts = stream_video_->getTimedts();
            int a_dts = stream_audio_->getTimedts();
            int max_dts = std::max(v_dts, a_dts);
            int min_dts = std::min(v_dts, a_dts);
            fmt1::print("seeking diff v{}-a{}={}\n", v_dts, a_dts, v_dts - a_dts);
            //一定时间以上才跳帧
            if (max_dts - min_dts > 100)
            {
                int sv = stream_video_->skipFrame(max_dts);
                int sa = stream_audio_->skipFrame(max_dts);
                fmt1::print("drop {} audio frames, {} video frames\n", sa, sv);
                /*v_dts = _videoStream->getTimedts();
                a_dts = _audioStream->getTimedts();
                fmt1::print("seeking end diff v%d-a%d=%d\n", v_dts, a_dts, v_dts - a_dts);*/
            }
        }
        //cout << "se"<<engine_->getTicks()-se << " "<<endl;
    }
    return 0;
}
/*
// 自定义 IO 回调函数
static int read_packet(void* opaque, uint8_t* buf, int buf_size) {
    SDL_IOStream* stream = static_cast<SDL_IOStream*>(opaque);
    return SDL_ReadIO(stream, buf, buf_size);
}

static int64_t seek(void *opaque, int64_t offset, int whence) {
    SDL_IOStream *stream = static_cast<SDL_IOStream*>(opaque);
    return SDL_SeekIO(stream, offset, static_cast<SDL_IOWhence>(whence));
}

int PotMedia::openFile(const std::string& filename) {
    // 1. 创建 SDL_IOStream 并初始化 AVIO
    SDL_IOStream* sdl_stream = SDL_IOFromFile(filename.c_str(), "rb");
    if (!sdl_stream) return -1;

    const int buffer_size = 65536;
    unsigned char* io_buffer = static_cast<unsigned char*>(av_malloc(buffer_size));
    AVIOContext* avio_ctx = avio_alloc_context(
            io_buffer, buffer_size, 0,
            sdl_stream, read_packet, nullptr, seek // 使用修正后的 seek
    );

    // 2. 主格式上下文初始化
    AVFormatContext* format_ctx = avformat_alloc_context();
    format_ctx->pb = avio_ctx;
    format_ctx->flags |= AVFMT_FLAG_CUSTOM_IO | AVFMT_FLAG_NOBUFFER;

    // 显式指定输入格式（根据实际文件类型调整）
    const AVInputFormat* input_fmt = av_find_input_format("mp4");
    int err = avformat_open_input(&format_ctx, nullptr, NULL, nullptr);
    if (err != 0) {
        char err_buf[1024];
        av_strerror(err, err_buf, sizeof(err_buf));
        SDL_Log("avformat_open_input failed: %s", err_buf);
        avio_context_free(&avio_ctx);
        SDL_CloseIO(sdl_stream);
        return -1;
    }

    // 3. 流信息探测与分离
    avformat_find_stream_info(format_ctx, nullptr);
    streams_.resize(format_ctx->nb_streams);

    // 分离视频/音频/字幕流（无需重复打开文件）
    for (int i = 0; i < format_ctx->nb_streams; i++) {
        AVStream* stream = format_ctx->streams[i];
        AVCodecParameters* codecpar = stream->codecpar;

        switch (codecpar->codec_type) {
            case MEDIA_TYPE_VIDEO: {
                PotStreamVideo* st = new PotStreamVideo();
                st->setStreamIndex(i);
                st->initFromCodecpar(codecpar);  // 需实现根据 codecpar 初始化的方法
                streams_[i] = st;
                if (!stream_video_->exist()) stream_video_ = st;;
                break;

            }
            case AVMEDIA_TYPE_AUDIO: {
                PotStreamAudio* st = new PotStreamAudio();
                st->setStreamIndex(i);
                st->initFromCodecpar(codecpar);
                streams_[i] = st;
                if (!stream_audio_->exist()) stream_audio_ = st;;
                break;
            }
            case AVMEDIA_TYPE_SUBTITLE: {
                PotStreamSubtitle* st = new PotStreamSubtitle();
                st->setStreamIndex(i);
                streams_[i] = st;
                if (!stream_subtitle_->exist()) stream_subtitle_ = st;
                break;
            }
        }
    }

    // 4. 资源释放逻辑
    //auto cleanup = [&]() {
    //    SDL_Log("测试8");
    //    avformat_close_input(&format_ctx);
    //    avio_context_free(&avio_ctx);
    //    SDL_CloseIO(sdl_stream);
    //    av_free(io_buffer);                 // 释放 av_malloc 分配的内存
    //};

    // 5. 总时长计算（示例逻辑）
    if (stream_audio_ && stream_audio_->exist()) {
        total_time_ = stream_audio_->getTotalTime();
        stream_audio_->openAudioDevice();
    } else if (stream_video_) {
        total_time_ = stream_video_->getTotalTime();
    }
    //cleanup();
    return 0;
}*/

int PotMedia::openFile(const std::string& filename)
{
    auto format_ctx = avformat_alloc_context();

    if (avformat_open_input(&format_ctx, filename.c_str(), nullptr, nullptr) == 0)
    {
        avformat_find_stream_info(format_ctx, nullptr);

        avformat_open_input(&format_ctx_video_, filename.c_str(), nullptr, nullptr);
        avformat_open_input(&format_ctx_audio_, filename.c_str(), nullptr, nullptr);
        avformat_open_input(&format_ctx_subtitle_, filename.c_str(), nullptr, nullptr);

        avformat_find_stream_info(format_ctx_video_, nullptr);
        avformat_find_stream_info(format_ctx_audio_, nullptr);
        avformat_find_stream_info(format_ctx_subtitle_, nullptr);

        streams_.resize(format_ctx->nb_streams);
        for (int i = 0; i < format_ctx->nb_streams; i++)
        {
            switch (format_ctx->streams[i]->codecpar->codec_type)
            {
            case MEDIA_TYPE_VIDEO:
            {
                auto st = new PotStreamVideo();
                st->setFormatCtx(format_ctx_video_);
                streams_[i] = st;
                if (!stream_video_->exist())
                {
                    stream_video_ = st;
                }
                break;
            }
            case MEDIA_TYPE_AUDIO:
            {
                auto st = new PotStreamAudio();
                st->setFormatCtx(format_ctx_audio_);
                streams_[i] = st;
                if (!stream_audio_->exist())
                {
                    stream_audio_ = st;
                }
                break;
            }
            }
            if (streams_[i])
            {
                streams_[i]->setStreamIndex(i);
                streams_[i]->openFile(filename);
            }
            //stream_index_vector_.push_back(i);
            //if (stream_index_vector_.size() == 1)
            //{
            //    //fmt1::print("finded media stream: %d\n", type);
            //    stream_ = format_ctx_->streams[i];
            //    codec_ctx_ = stream_->codec;
            //    if (stream_->r_frame_rate.den)
            //    {
            //        time_per_frame_ = 1e3 / av_q2d(stream_->r_frame_rate);
            //    }
            //    time_base_packet_ = 1e3 * av_q2d(stream_->time_base);
            //    total_time_ = format_ctx_->duration * 1e3 / AV_TIME_BASE;
            //    start_time_ = format_ctx_->start_time * 1e3 / AV_TIME_BASE;
            //    codec_ = avcodec_find_decoder(codec_ctx_->codec_id);
            //    avcodec_open2(codec_ctx_, codec_, nullptr);
            //}
            //}
        }
    }
    avformat_close_input(&format_ctx);

    if (stream_audio_->exist())
    {
        total_time_ = stream_audio_->getTotalTime();
        stream_audio_->openAudioDevice();
    }
    else
    {
        total_time_ = stream_video_->getTotalTime();
    }
    return 0;
}

int PotMedia::getAudioTime()
{
    //fmt1::print("\t\t\t\t\t\t\r%d,%d,%d", audioStream->time, videoStream->time, audioStream->getAudioTime());
    return stream_audio_->getTime();
}

int PotMedia::getVideoTime()
{
    return stream_video_->getTime();
}

int PotMedia::seekTime(int time, int direct /*= 1*/, int reset /*= 0*/)
{
    time = std::min(time, total_time_ - 100);
    //stream_video_->seek(time, direct, reset);
    stream_video_->seek(time, direct, 1);
    stream_audio_->seek(time, direct, reset);
    //stream_subtitle_->seek(time - 5000, direct, reset);

    seeking_ = true;
    stream_audio_->resetDecodeState();
    return 0;
}

int PotMedia::seekPos(double pos, int direct /*= 1*/, int reset /*= 0*/)
{
    //fmt1::print("\nseek %f pos, %f s\n", pos, pos * totalTime / 1e3);
    return seekTime(pos * total_time_, direct, reset);
}

int PotMedia::showVideoFrame(int time)
{
    return stream_video_->show(time);
}

int PotMedia::getTime()
{
    if (stream_audio_->exist())
    {
        return stream_audio_->getTime();
    }
    else
    {
        return stream_video_->getTime();
    }
}

void PotMedia::destroy()
{
}

bool PotMedia::isMedia()
{
    if (streams_.empty())
    {
        return false;
    }
    return stream_audio_->exist() || stream_video_->exist();
}

void PotMedia::setPause(bool pause)
{
    stream_audio_->setPause(pause);
    stream_video_->setPause(pause);
    //stream_subtitle_->setPause(pause);
}

void PotMedia::switchStream(PotMediaType mt)
{
    if (getStreamCount(mt) <= 1)
    {
        return;
    }
    int current_index = -1;
    for (int i = 0; i < streams_.size(); i++)
    {
        if (streams_[i] && streams_[i]->getType() == mt
            && (streams_[i] == stream_video_ || streams_[i] == stream_audio_))
        {
            current_index = i;
            break;
        }
    }
    if (current_index == -1)
    {
        return;
    }
    PotStream* st = streams_[current_index];
    for (int i = current_index + 1; i < streams_.size(); i++)
    {
        if (streams_[i] && streams_[i]->getType() == mt)
        {
            st = streams_[i];
            break;
        }
    }
    if (st == streams_[current_index])
    {
        for (int i = 0; i < current_index; i++)
        {
            if (streams_[i] && streams_[i]->getType() == mt)
            {
                st = streams_[i];
                break;
            }
        }
    }

    switch (mt)
    {
    case MEDIA_TYPE_VIDEO:
    {
        stream_video_ = (PotStreamVideo*)st;
        break;
    }
    case MEDIA_TYPE_AUDIO:
    {
        //注意此处效果不佳
        //若假设所有音频使用同一解码器，则切换的效果会较好
        int t = getTime();
        stream_audio_->closeAudioDevice();
        //stream_audio_->clearMap();
        stream_audio_ = (PotStreamAudio*)st;
        stream_audio_->openAudioDevice();
        stream_audio_->seek(t);
        //stream_audio_->setStreamIndex(st->getStreamIndex());
        break;
    }
    }
}

int PotMedia::getStreamCount(PotMediaType mt)
{
    int count = 0;
    for (int i = 0; i < streams_.size(); i++)
    {
        if (streams_[i] && streams_[i]->getType() == mt && streams_[i]->exist())
        {
            count++;
        }
    }
    return count;
}
