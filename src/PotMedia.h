#pragma once

#include "PotBase.h"
#include "PotStream.h"
#include "PotStreamAudio.h"
#include "PotStreamVideo.h"

class PotMedia : public PotBase
{
public:
    PotMedia();
    virtual ~PotMedia();

private:
    std::vector<PotStream*> streams_;

    AVFormatContext* format_ctx_video_, * format_ctx_audio_, * format_ctx_subtitle_;

    PotStreamVideo* stream_video_ = nullptr;
    PotStreamAudio* stream_audio_ = nullptr;

    PotStreamVideo blank_video_;
    PotStreamAudio blank_audio_;

private:
    int count_ = 0;
    int total_time_ = 0;
    int lastdts_ = 0;
    int timebase_ = 0;
    bool seeking_ = false;

public:
    PotStreamVideo* getVideo() { return stream_video_; }
    PotStreamAudio* getAudio() { return stream_audio_; }
    int decodeFrame();
    int openFile(const std::string& filename);
    int getAudioTime();
    int getVideoTime();
    int seekTime(int time, int direct = 1, int reset = 0);
    int seekPos(double pos, int direct = 1, int reset = 0);
    int showVideoFrame(int time);
    int getTotalTime() { return total_time_; }
    int getTime();
    void destroy();
    bool isMedia();
    void setPause(bool pause);
    void switchStream(PotMediaType mt);
    int getStreamCount(PotMediaType mt);
};
