#include "Encoder.h"
#include <iostream>
#include <cstring>

Encoder::Encoder(const std::string& filename, int fps)
    : outputFile(filename), frameRate(fps) {
}

Encoder::~Encoder() {
    finish();
}

bool Encoder::init() {
    avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr, outputFile.c_str());
    if (!fmtCtx) {
        std::cerr << "❌ Failed to create output context\n";
        return false;
    }

    const AVCodec* vCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!vCodec) {
        std::cerr << "❌ Could not find H264 encoder\n";
        return false;
    }

    videoStream = avformat_new_stream(fmtCtx, vCodec);
    if (!videoStream) {
        std::cerr << "❌ Could not create video stream\n";
        return false;
    }

    videoCodecCtx = avcodec_alloc_context3(vCodec);
    videoCodecCtx->width = 1920;
    videoCodecCtx->height = 1080;
    videoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    videoCodecCtx->time_base = { 1, frameRate };
    videoCodecCtx->framerate = { frameRate, 1 };
    videoCodecCtx->bit_rate = 4000000;
    if (fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        videoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(videoCodecCtx, vCodec, nullptr) < 0) {
        std::cerr << "❌ Could not open video codec\n";
        return false;
    }
    avcodec_parameters_from_context(videoStream->codecpar, videoCodecCtx);

    // Audio stream
    const AVCodec* aCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!aCodec) {
        std::cerr << "❌ Could not find AAC encoder\n";
        return false;
    }

    audioStream = avformat_new_stream(fmtCtx, aCodec);
    if (!audioStream) {
        std::cerr << "❌ Could not create audio stream\n";
        return false;
    }

    audioCodecCtx = avcodec_alloc_context3(aCodec);
    av_channel_layout_default(&audioCodecCtx->ch_layout, 2);
    audioCodecCtx->sample_rate = 48000;
    audioCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    audioCodecCtx->time_base = { 1, audioCodecCtx->sample_rate };
    audioCodecCtx->bit_rate = 128000;
    if (fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        audioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(audioCodecCtx, aCodec, nullptr) < 0) {
        std::cerr << "❌ Could not open audio codec\n";
        return false;
    }
    avcodec_parameters_from_context(audioStream->codecpar, audioCodecCtx);

    // Open file
    if (!(fmtCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&fmtCtx->pb, outputFile.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "❌ Could not open output file\n";
            return false;
        }
    }

    if (avformat_write_header(fmtCtx, nullptr) < 0) {
        std::cerr << "❌ Failed to write header\n";
        return false;
    }

    swsCtx = sws_getContext(
        videoCodecCtx->width, videoCodecCtx->height, AV_PIX_FMT_BGRA,
        videoCodecCtx->width, videoCodecCtx->height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    swrCtx = swr_alloc();
    av_opt_set_chlayout(swrCtx, "in_chlayout", &audioCodecCtx->ch_layout, 0);
    av_opt_set_int(swrCtx, "in_sample_rate", 48000, 0);
    av_opt_set_sample_fmt(swrCtx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_chlayout(swrCtx, "out_chlayout", &audioCodecCtx->ch_layout, 0);
    av_opt_set_int(swrCtx, "out_sample_rate", audioCodecCtx->sample_rate, 0);
    av_opt_set_sample_fmt(swrCtx, "out_sample_fmt", audioCodecCtx->sample_fmt, 0);
    swr_init(swrCtx);

    initialized = true;
    std::cout << "✅ Encoder initialized\n";
    return true;
}
