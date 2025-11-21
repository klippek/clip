#pragma once
#include "Encoder.h"
#include "AudioCapture.h"
#include <thread>

class CaptureManager {
public:
    CaptureManager();
    ~CaptureManager();

    bool startCapture();
    void stopCapture();

private:
    void initEncoder();
    void finishEncoding();
    bool captureFrame();

    Encoder* encoder = nullptr;
    AudioCapture systemAudio;
    AudioCapture micAudio;

    std::thread captureThread;
    bool capturing = false;
    const int frameRate = 60;
    const std::string outputFile = "output.mp4";
};
