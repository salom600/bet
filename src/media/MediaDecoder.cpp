#include "media/MediaDecoder.h"

#include <QDebug>
#include <QImage>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/channel_layout.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <algorithm>
#include <cmath>

namespace ve {

// ---------------------------------------------------------------------------
// Utility: AVRational → double
// ---------------------------------------------------------------------------
static inline double rat2d(const AVRational& r) {
    return r.den ? static_cast<double>(r.num) / r.den : 0.0;
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------
MediaDecoder::MediaDecoder() = default;

MediaDecoder::~MediaDecoder() {
    close();
}

void MediaDecoder::close() {
    if (swsCtx_) { sws_freeContext(swsCtx_); swsCtx_ = nullptr; }
    if (swrCtx_) { swr_free(&swrCtx_); }
    if (videoCodec_) { avcodec_free_context(&videoCodec_); }
    if (audioCodec_) { avcodec_free_context(&audioCodec_); }
    if (fmtCtx_)     { avformat_close_input(&fmtCtx_); }
    videoStream_ = -1;
    audioStream_ = -1;
    path_.clear();
}

// ---------------------------------------------------------------------------
// Probe (static, no need to keep file open)
// ---------------------------------------------------------------------------
MediaInfo MediaDecoder::probe(const QString& path) {
    MediaInfo info;
    AVFormatContext* fmt = nullptr;
    const std::string p = path.toUtf8().constData();

    if (avformat_open_input(&fmt, p.c_str(), nullptr, nullptr) < 0) {
        return info;
    }
    if (avformat_find_stream_info(fmt, nullptr) < 0) {
        avformat_close_input(&fmt);
        return info;
    }

    // AVFormatContext::duration is int64_t microseconds (or AV_NOPTS_VALUE).
    if (fmt->duration > 0 && fmt->duration != AV_NOPTS_VALUE) {
        info.duration = static_cast<double>(fmt->duration) / AV_TIME_BASE;
    }

    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        AVStream* st = fmt->streams[i];
        AVCodecParameters* par = st->codecpar;
        if (par->codec_type == AVMEDIA_TYPE_VIDEO && !info.hasVideo) {
            info.hasVideo = true;
            info.width    = par->width;
            info.height   = par->height;
            info.fpsNum   = st->avg_frame_rate.num;
            info.fpsDen   = st->avg_frame_rate.den ? st->avg_frame_rate.den : 1;
            const AVCodecDescriptor* d = avcodec_descriptor_get(par->codec_id);
            info.codec = d ? QString::fromUtf8(d->name) : "video";
        } else if (par->codec_type == AVMEDIA_TYPE_AUDIO && !info.hasAudio) {
            info.hasAudio        = true;
            info.audioChannels   = par->ch_layout.nb_channels;
            info.audioSampleRate = par->sample_rate;
        }
    }
    avformat_close_input(&fmt);
    return info;
}

// ---------------------------------------------------------------------------
// Open file for sequential decoding
// ---------------------------------------------------------------------------
bool MediaDecoder::open(const QString& path) {
    close();
    path_ = path;
    const std::string p = path.toUtf8().constData();

    if (avformat_open_input(&fmtCtx_, p.c_str(), nullptr, nullptr) < 0) {
        qWarning() << "MediaDecoder: cannot open" << path;
        return false;
    }
    if (avformat_find_stream_info(fmtCtx_, nullptr) < 0) {
        qWarning() << "MediaDecoder: no stream info" << path;
        close();
        return false;
    }

    for (unsigned i = 0; i < fmtCtx_->nb_streams; ++i) {
        AVStream* st = fmtCtx_->streams[i];
        AVCodecParameters* par = st->codecpar;
        const AVCodec* codec = avcodec_find_decoder(par->codec_id);
        if (!codec) continue;

        if (par->codec_type == AVMEDIA_TYPE_VIDEO && videoStream_ < 0) {
            videoStream_ = static_cast<int>(i);
            videoCodec_  = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(videoCodec_, par);
            avcodec_open2(videoCodec_, codec, nullptr);
        } else if (par->codec_type == AVMEDIA_TYPE_AUDIO && audioStream_ < 0) {
            audioStream_ = static_cast<int>(i);
            audioCodec_  = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(audioCodec_, par);
            avcodec_open2(audioCodec_, codec, nullptr);
        }
    }

    if (videoStream_ < 0 && audioStream_ < 0) {
        qWarning() << "MediaDecoder: no decodable streams in" << path;
        close();
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Grab a video frame at a given timestamp
// ---------------------------------------------------------------------------
QImage MediaDecoder::grabFrame(double atSeconds, int maxWidth, int maxHeight) {
    if (!fmtCtx_ || videoStream_ < 0 || !videoCodec_) return QImage();

    const int64_t seekTarget = static_cast<int64_t>(atSeconds * AV_TIME_BASE);
    avformat_seek_file(fmtCtx_, -1, INT64_MIN, seekTarget, INT64_MAX, 0);
    avcodec_flush_buffers(videoCodec_);

    AVPacket* pkt = av_packet_alloc();
    AVFrame*  frame = av_frame_alloc();
    QImage result;

    // Track SWS context parameters so we recreate on dimension change.
    int lastW = 0, lastH = 0, lastFW = 0, lastFH = 0;
    AVPixelFormat lastF = AV_PIX_FMT_NONE;

    while (av_read_frame(fmtCtx_, pkt) >= 0) {
        if (pkt->stream_index != videoStream_) {
            av_packet_unref(pkt);
            continue;
        }
        int ret = avcodec_send_packet(videoCodec_, pkt);
        av_packet_unref(pkt);
        if (ret < 0) continue;

        while (avcodec_receive_frame(videoCodec_, frame) == 0) {
            int w = frame->width;
            int h = frame->height;
            if (maxWidth > 0 && maxHeight > 0) {
                double sx = static_cast<double>(maxWidth)  / w;
                double sy = static_cast<double>(maxHeight) / h;
                double s  = std::min(sx, sy);
                if (s < 1.0) {
                    w = std::max(1, static_cast<int>(std::floor(w * s)));
                    h = std::max(1, static_cast<int>(std::floor(h * s)));
                }
            }
            const AVPixelFormat srcFmt = static_cast<AVPixelFormat>(frame->format);
            if (lastW != w || lastH != h || lastFW != frame->width ||
                lastFH != frame->height || lastF != srcFmt) {
                if (swsCtx_) sws_freeContext(swsCtx_);
                swsCtx_ = sws_getContext(frame->width, frame->height, srcFmt,
                                         w, h, AV_PIX_FMT_RGB24,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);
                lastW = w; lastH = h;
                lastFW = frame->width; lastFH = frame->height;
                lastF = srcFmt;
            }
            if (swsCtx_) {
                QImage img(w, h, QImage::Format_RGB888);
                uint8_t* dst[4] = { img.bits(), nullptr, nullptr, nullptr };
                int dstLinesize[4] = { img.bytesPerLine(), 0, 0, 0 };
                sws_scale(swsCtx_, frame->data, frame->linesize, 0, h, dst, dstLinesize);
                result = img.copy();
            }
            av_frame_unref(frame);
            goto done;
        }
    }

done:
    av_frame_free(&frame);
    av_packet_free(&pkt);
    return result;
}

// ---------------------------------------------------------------------------
// Extract audio waveform peaks
// ---------------------------------------------------------------------------
std::vector<float> MediaDecoder::audioPeaks(int peaksCount) {
    std::vector<float> peaks;
    if (!fmtCtx_ || audioStream_ < 0 || !audioCodec_ || peaksCount <= 0) return peaks;

    // Configure SwrContext to convert to mono F32
    AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_MONO;
    AVChannelLayout inLayout   = audioCodec_->ch_layout;
    if (swr_alloc_set_opts2(&swrCtx_, &outLayout, AV_SAMPLE_FMT_FLT,
                             audioCodec_->sample_rate,
                             &inLayout, audioCodec_->sample_fmt,
                             audioCodec_->sample_rate, 0, nullptr) < 0) {
        qWarning() << "MediaDecoder: swr_alloc_set_opts2 failed";
        return peaks;
    }
    if (swr_init(swrCtx_) < 0) {
        qWarning() << "MediaDecoder: swr_init failed";
        return peaks;
    }

    double dur = 0.0;
    if (fmtCtx_->duration > 0 && fmtCtx_->duration != AV_NOPTS_VALUE) {
        dur = static_cast<double>(fmtCtx_->duration) / AV_TIME_BASE;
    }
    int64_t totalSamples = static_cast<int64_t>(audioCodec_->sample_rate * dur);
    if (totalSamples <= 0) totalSamples = peaksCount * 256; // fallback

    int64_t samplesPerPeak = std::max<int64_t>(1, totalSamples / peaksCount);

    peaks.assign(peaksCount, 0.0f);

    AVPacket* pkt = av_packet_alloc();
    AVFrame*  frame = av_frame_alloc();

    int64_t sampleIdx = 0;
    while (av_read_frame(fmtCtx_, pkt) >= 0) {
        if (pkt->stream_index != audioStream_) {
            av_packet_unref(pkt);
            continue;
        }
        int ret = avcodec_send_packet(audioCodec_, pkt);
        av_packet_unref(pkt);
        if (ret < 0) continue;

        while (avcodec_receive_frame(audioCodec_, frame) == 0) {
            const int inSamples = frame->nb_samples;
            const int outSamples = swr_get_out_samples(swrCtx_, inSamples);
            std::vector<float> buf(static_cast<size_t>(outSamples), 0.0f);
            uint8_t* outPtr = reinterpret_cast<uint8_t*>(buf.data());
            int got = swr_convert(swrCtx_, &outPtr, outSamples,
                                  const_cast<const uint8_t**>(frame->data),
                                  inSamples);
            for (int i = 0; i < got; ++i) {
                int64_t idx = sampleIdx / samplesPerPeak;
                if (idx >= peaksCount) break;
                float v = std::fabs(buf[static_cast<size_t>(i)]);
                if (v > peaks[static_cast<size_t>(idx)])
                    peaks[static_cast<size_t>(idx)] = v;
                sampleIdx++;
            }
            av_frame_unref(frame);
        }
    }
    av_frame_free(&frame);
    av_packet_free(&pkt);
    return peaks;
}

} // namespace ve
