#include "AudioCapture.h"
#include <iostream>

AudioCapture::~AudioCapture() {
    stop();
    if (fmtCtx) avformat_close_input(&fmtCtx);
    if (packet) av_packet_free(&packet);
    if (codecCtx) avcodec_free_context(&codecCtx);
}

bool AudioCapture::init(bool loopback, const std::string& deviceName, int rate, int ch) {
    sampleRate = rate;
    channels = ch;

    const AVInputFormat* inputFormat = nullptr;
    if (loopback)
        inputFormat = av_find_input_format("wasapi");
    else
        inputFormat = av_find_input_format("dshow");

    if (!inputFormat) {
        std::cerr << "❌ Could not find input format\n";
        return false;
    }

    if (avformat_open_input(&fmtCtx, deviceName.c_str(), inputFormat, nullptr) < 0) {
        std::cerr << "❌ Failed to open audio device: " << deviceName << "\n";
        return false;
    }

    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        std::cerr << "❌ Could not find stream info\n";
        return false;
    }

    audioStreamIndex = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIndex < 0) {
        std::cerr << "❌ No audio stream found\n";
        return false;
    }

    AVStream* stream = fmtCtx->streams[audioStreamIndex];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        std::cerr << "❌ Could not find decoder\n";
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, stream->codecpar);
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        std::cerr << "❌ Could not open codec\n";
        return false;
    }

    packet = av_packet_alloc();
    std::cout << "✅ AudioCapture initialized for " << (loopback ? "system audio" : "microphone") << " (" << deviceName << ")\n";
    return true;
}

void AudioCapture::start() {
    capturing = true;
    captureThread = std::thread(&AudioCapture::captureLoop, this);
}

void AudioCapture::stop() {
    capturing = false;
    if (captureThread.joinable()) captureThread.join();
}

void AudioCapture::captureLoop() {
    while (capturing) {
        if (av_read_frame(fmtCtx, packet) < 0) continue;
        if (packet->stream_index != audioStreamIndex) {
            av_packet_unref(packet);
            continue;
        }

        std::lock_guard<std::mutex> lock(bufferMutex);
        audioBuffer.insert(audioBuffer.end(), packet->data, packet->data + packet->size);
        av_packet_unref(packet);
    }
}

bool AudioCapture::readAudioFrames(std::vector<uint8_t>& buffer) {
    std::lock_guard<std::mutex> lock(bufferMutex);
    if (audioBuffer.empty()) return false;
    buffer = std::move(audioBuffer);
    audioBuffer.clear();
    return true;
}
