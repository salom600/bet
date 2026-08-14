/*
 * VideoEditor - BinClip.h
 * Source media descriptor that lives in the bin.
 *
 * Adapted from Kdenlive's src/bin/projectclip.h.
 *
 * A BinClip represents a media file on disk (video, image, audio). It owns:
 *   - the probed MediaInfo (resolution, fps, duration, ...)
 *   - a small set of cached thumbnails
 *   - a list of timeline clip IDs that reference it (refcount-style)
 *
 * When the user drags a bin clip onto the timeline, a ClipModel instance is
 * created that REFERENCES this BinClip by id. Multiple timeline instances
 * share the same BinClip.
 */
#pragma once

#include "../definitions.h"
#include "../media/MediaBackend.h"
#include "../utils/GenTime.h"
#include <QObject>
#include <QImage>
#include <QString>
#include <QUuid>
#include <QList>
#include <memory>

namespace ve {

class BinModel;

class BinClip : public QObject {
    Q_OBJECT
public:
    BinClip(QString id, QString sourcePath, ClipType type, MediaInfo info, QObject* parent = nullptr);

    const QString& id()         const { return id_; }
    const QString& sourcePath() const { return sourcePath_; }
    ClipType       type()       const { return type_; }
    const MediaInfo& info()     const { return info_; }

    QString name() const;
    void    setName(const QString& n) { name_ = n; emit changed(); }

    /// Duration of the source media, in seconds.
    double duration() const { return info_.duration; }

    /// Reference counting: timeline clips register/deregister themselves.
    void addRef(ObjectId clipId);
    void removeRef(ObjectId clipId);
    QList<ObjectId> referrers() const { return referrers_; }

    /// Thumbnail management. Returns cached or loads via backend.
    QImage thumbnail(double atSeconds = 0.5) const;
    void   setThumbnail(double atSeconds, const QImage& img);

    /// Audio waveform peaks (cached after first computation).
    std::vector<float> audioPeaks() const;
    void               setAudioPeaks(std::vector<float> peaks) const;

signals:
    void changed();
    void refCountChanged();

private:
    QString id_;
    QString sourcePath_;
    QString name_;
    ClipType type_ = ClipType::Unknown;
    MediaInfo info_;
    QList<ObjectId> referrers_;
    mutable QImage thumbnail_;
    mutable bool   thumbnailLoaded_ = false;
    mutable std::vector<float> audioPeaks_;
    mutable bool   audioPeaksLoaded_ = false;
};

} // namespace ve
