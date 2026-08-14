#pragma once

#include <QObject>
#include <QImage>
#include <QString>
#include <memory>

// Forward FFmpeg types to keep this header clean.
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct SwrContext;

namespace ve {

/// Probe result describing a media file.
struct MediaInfo {
    bool   hasVideo      = false;
    bool   hasAudio      = false;
    int    width         = 0;
    int    height        = 0;
    int    fpsNum        = 0;
    int    fpsDen        = 1;
    double duration      = 0.0; // seconds
    int    audioChannels = 0;
    int    audioSampleRate = 0;
    QString codec;
};

/// FFmpeg-backed decoder for:
///  - probing media files (MediaInfo)
///  - extracting a single video frame at a timestamp (thumbnail / preview)
///  - extracting a downsampled audio waveform (peaks)
class MediaDecoder {
public:
    MediaDecoder();
    ~MediaDecoder();

    MediaDecoder(const MediaDecoder&) = delete;
    MediaDecoder& operator=(const MediaDecoder&) = delete;

    /// Probe a file (no need to call open()).
    static MediaInfo probe(const QString& path);

    /// Open a file for sequential frame/waveform extraction.
    bool open(const QString& path);
    void close();
    bool isOpen() const { return fmtCtx_ != nullptr; }

    /// Seek to given timestamp (seconds) in the video stream and decode the
    /// next available video frame, converted to RGB24. Returns null QImage on failure.
    QImage grabFrame(double atSeconds, int maxWidth = 0, int maxHeight = 0);

    /// Read raw audio samples for the entire file and return |peaks| downsampled
    /// peak values (0..1) suitable for waveform rendering.
    std::vector<float> audioPeaks(int peaksCount = 400);

private:
    QString path_;
    AVFormatContext* fmtCtx_       = nullptr;
    AVCodecContext*  videoCodec_   = nullptr;
    AVCodecContext*  audioCodec_   = nullptr;
    SwsContext*      swsCtx_       = nullptr;
    SwrContext*      swrCtx_       = nullptr;
    int              videoStream_  = -1;
    int              audioStream_  = -1;
};

} // namespace ve
