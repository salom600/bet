/*
 * VideoEditor - MediaBackend.h
 * Abstract interface for media probing and decoding.
 *
 * This decouples the model layer from any specific media framework.
 * Currently we ship an FFmpegBackend; in future we could ship an
 * MLTBackend that delegates to libmlt++ without changing call sites.
 *
 * Modeled on Kdenlive's MltConnection + ClipController split.
 */
#pragma once

#include <QString>
#include <QImage>
#include <vector>
#include <memory>

namespace ve {

struct MediaInfo {
    bool   hasVideo       = false;
    bool   hasAudio       = false;
    int    width          = 0;
    int    height         = 0;
    double fps            = 0.0;
    double duration       = 0.0;  // seconds
    int    audioChannels  = 0;
    int    sampleRate     = 0;
    QString codec;
    QString pixelFormat;
};

class MediaBackend {
public:
    virtual ~MediaBackend() = default;

    /// Probe a file without opening a decoder. Cheap.
    virtual MediaInfo probe(const QString& path) = 0;

    /// Open file for sequential frame / audio extraction.
    virtual bool open(const QString& path) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    /// Seek to the given time and decode the next video frame.
    /// Returns null QImage on failure.
    virtual QImage grabFrame(double atSeconds, int maxWidth = 0, int maxHeight = 0) = 0;

    /// Read raw audio samples and downsample to |peaksCount| peak values
    /// in range [0.0, 1.0]. Used for waveform rendering.
    virtual std::vector<float> audioPeaks(int peaksCount = 400) = 0;
};

/// Factory: returns a sensible default backend (FFmpegBackend).
std::unique_ptr<MediaBackend> createDefaultBackend();

} // namespace ve
