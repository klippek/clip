#include "CaptureManager.h"
#include "AudioCapture.h"
#include "Encoder.h"
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>

CaptureManager::CaptureManager() {
    std::cout << "CaptureManager created\n";
}

CaptureManager::~CaptureManager() {
    stopCapture();
    if (encoder) delete encoder;
    std::cout << "CaptureManager destroyed\n";
}

bool CaptureManager::startCapture() {
    std::cout << "?? Starting capture...\n";
    if (capturing) return false;
    capturing = true;

    initEncoder();

    bool sysOK = systemAudio.init(true, "audio=default:loopback", 48000, 2);
    bool micOK = micAudio.init(false, "audio=default", 48000, 2);

    if (!sysOK || !micOK) std::cerr << "? Could not initialize audio devices.\n";
    else {
        systemAudio.start();
        micAudio.start();
    }

    captureThread = std::thread([this]() {
        using namespace std::chrono;
        auto startTime = high_resolution_clock::now();
        const auto duration = seconds(30);

        while (capturing && high_resolution_clock::now() - startTime < duration) {
            captureFrame();
            std::this_thread::sleep_for(milliseconds(1000 / frameRate));
        }
        finishEncoding();
        });

    return true;
}

void CaptureManager::stopCapture() {
    capturing = false;
    systemAudio.stop();
    micAudio.stop();
    if (captureThread.joinable()) captureThread.join();
    std::cout << "Recording stopped\n";
}

void CaptureManager::initEncoder() {
    encoder = new Encoder(outputFile, frameRate);
    encoder->init();
    std::cout << "Encoder initialized\n";
}

void CaptureManager::finishEncoding() {
    if (encoder) encoder->finish();
    std::cout << "Recording complete\n";
}

bool CaptureManager::captureFrame() {
    std::vector<uint8_t> sysBuf, micBuf;
    if (systemAudio.readAudioFrames(sysBuf)) encoder->encodeAudioFrame(sysBuf);
    if (micAudio.readAudioFrames(micBuf)) encoder->encodeAudioFrame(micBuf);

    // Dummy video frame (replace with real capture)
    std::vector<uint8_t> dummyFrame(1920 * 1080 * 4, 128);
    encoder->encodeVideoFrame(dummyFrame);

    return true;
}
