#pragma once
#include <string>
#include <vector>
#include <mutex>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

class Encoder {
public:
    Encoder(const std::string& filename, int fps);
    ~Encoder();

    bool init();
    void encodeVideoFrame(const std::vector<uint8_t>& frameData);
    void encodeAudioFrame(const std::vector<uint8_t>& audioData);
    void finish();

private:
    std::string outputFile;
    int frameRate = 60;
    bool initialized = false;

    AVFormatContext* fmtCtx = nullptr;
    AVCodecContext* videoCodecCtx = nullptr;
    AVCodecContext* audioCodecCtx = nullptr;
    AVStream* videoStream = nullptr;
    AVStream* audioStream = nullptr;

    SwsContext* swsCtx = nullptr;
    SwrContext* swrCtx = nullptr;

    int64_t videoPts = 0;
    int64_t audioPts = 0;

    std::mutex encodeMutex;
};
