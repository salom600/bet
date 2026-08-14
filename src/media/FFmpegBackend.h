/*
 * VideoEditor - FFmpegBackend.h
 * MediaBackend implementation using libavformat/libavcodec/libswscale/libswresample.
 *
 * This is a refactor of the previous MediaDecoder — now implementing the
 * MediaBackend interface so the model layer never sees FFmpeg types.
 */
#pragma once

#include "media/MediaBackend.h"

// Forward FFmpeg types so the header doesn't pull them into the model layer.
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct SwrContext;

namespace ve {

class FFmpegBackend : public MediaBackend {
public:
    FFmpegBackend();
    ~FFmpegBackend() override;

    FFmpegBackend(const FFmpegBackend&) = delete;
    FFmpegBackend& operator=(const FFmpegBackend&) = delete;

    MediaInfo probe(const QString& path) override;
    bool open(const QString& path) override;
    void close() override;
    bool isOpen() const override { return fmtCtx_ != nullptr; }

    QImage grabFrame(double atSeconds, int maxWidth = 0, int maxHeight = 0) override;
    std::vector<float> audioPeaks(int peaksCount = 400) override;

private:
    QString path_;
    AVFormatContext* fmtCtx_      = nullptr;
    AVCodecContext*  videoCodec_  = nullptr;
    AVCodecContext*  audioCodec_  = nullptr;
    SwsContext*      swsCtx_      = nullptr;
    SwrContext*      swrCtx_      = nullptr;
    int              videoStream_ = -1;
    int              audioStream_ = -1;
    int              lastSwsW_    = 0;
    int              lastSwsH_    = 0;
    int              lastSwsFW_   = 0;
    int              lastSwsFH_   = 0;
    int              lastSwsFmt_  = 0;
};

} // namespace ve
