#include "model/BinClip.h"
#include "media/MediaBackend.h"
#include "media/ThumbnailCache.h"
#include <QFileInfo>
#include <QPainter>

namespace ve {

BinClip::BinClip(QString id, QString sourcePath, ClipType type, MediaInfo info, QObject* parent)
    : QObject(parent)
    , id_(std::move(id))
    , sourcePath_(std::move(sourcePath))
    , name_(QFileInfo(sourcePath_).completeBaseName())
    , type_(type)
    , info_(info)
{
}

QString BinClip::name() const {
    return name_;
}

void BinClip::addRef(ObjectId clipId) {
    if (!referrers_.contains(clipId)) {
        referrers_.append(clipId);
        emit refCountChanged();
    }
}

void BinClip::removeRef(ObjectId clipId) {
    if (referrers_.removeOne(clipId)) {
        emit refCountChanged();
    }
}

QImage BinClip::thumbnail(double atSeconds) const {
    if (thumbnailLoaded_) return thumbnail_;
    // Try disk cache first
    QImage cached = ThumbnailCache::self().get(sourcePath_, atSeconds);
    if (!cached.isNull()) {
        thumbnail_ = cached;
        thumbnailLoaded_ = true;
        return thumbnail_;
    }
    // Generate via backend
    auto backend = createDefaultBackend();
    if (type_ == ClipType::Image) {
        QImage img(sourcePath_);
        if (!img.isNull()) {
            thumbnail_ = img.scaled(160, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            ThumbnailCache::self().put(sourcePath_, 0.0, thumbnail_);
            thumbnailLoaded_ = true;
        }
    } else if (type_ == ClipType::Video || type_ == ClipType::AV) {
        if (backend->open(sourcePath_)) {
            QImage frame = backend->grabFrame(atSeconds, 160, 90);
            if (!frame.isNull()) {
                thumbnail_ = frame;
                ThumbnailCache::self().put(sourcePath_, atSeconds, thumbnail_);
                thumbnailLoaded_ = true;
            }
        }
    } else if (type_ == ClipType::Audio) {
        // Render a waveform preview thumbnail
        auto peaks = audioPeaks();
        if (!peaks.empty()) {
            QImage img(200, 60, QImage::Format_ARGB32);
            img.fill(Qt::transparent);
            QPainter p(&img);
            p.setPen(QColor(80, 200, 255));
            for (int i = 0; i < (int)peaks.size(); ++i) {
                int h = std::clamp(static_cast<int>(peaks[i] * img.height()), 0, img.height());
                int y0 = (img.height() - h) / 2;
                p.drawLine(i, y0, i, y0 + h);
            }
            thumbnail_ = img;
            thumbnailLoaded_ = true;
        }
    }
    return thumbnail_;
}

void BinClip::setThumbnail(double atSeconds, const QImage& img) {
    thumbnail_ = img;
    thumbnailLoaded_ = true;
    ThumbnailCache::self().put(sourcePath_, atSeconds, img);
    emit changed();
}

std::vector<float> BinClip::audioPeaks() const {
    if (audioPeaksLoaded_) return audioPeaks_;
    auto backend = createDefaultBackend();
    if (backend->open(sourcePath_)) {
        audioPeaks_ = backend->audioPeaks(400);
        audioPeaksLoaded_ = true;
    }
    return audioPeaks_;
}

void BinClip::setAudioPeaks(std::vector<float> peaks) const {
    audioPeaks_ = std::move(peaks);
    audioPeaksLoaded_ = true;
}

} // namespace ve
