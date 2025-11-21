#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <thread>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class AudioCapture {
public:
    AudioCapture() = default;
    ~AudioCapture();

    bool init(bool loopback, const std::string& deviceName, int rate, int channels);
    void start();
    void stop();
    bool readAudioFrames(std::vector<uint8_t>& buffer);

private:
    void captureLoop();

    AVFormatContext* fmtCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    AVPacket* packet = nullptr;
    int audioStreamIndex = -1;

    std::vector<uint8_t> audioBuffer;
    std::mutex bufferMutex;
    std::thread captureThread;
    bool capturing = false;

    int sampleRate = 48000;
    int channels = 2;
};
