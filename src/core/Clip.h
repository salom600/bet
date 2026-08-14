#pragma once

#include <QObject>
#include <QString>
#include <QUuid>
#include <QImage>
#include <vector>

namespace ve {

/// Type of media a clip references.
enum class MediaType {
    Video,
    Image,
    Audio
};

/// A single clip placed on a track.
/// A clip references a media file and specifies:
///  - which slice of the source to use (inSource / outSource in seconds)
///  - where on the timeline it sits (timelineStart in seconds)
///  - transform parameters (position, scale, opacity) for video/image
///  - audio parameters (volume, pan) for audio
class Clip : public QObject {
    Q_OBJECT
public:
    explicit Clip(QObject* parent = nullptr);

    // Identity
    QString id() const { return id_; }
    void setId(const QString& id) { id_ = id; }

    // Media source
    const QString& sourcePath() const { return sourcePath_; }
    void setSourcePath(const QString& p) { sourcePath_ = p; }

    MediaType type() const { return type_; }
    void setType(MediaType t) { type_ = t; }

    // Source range (seconds within source file)
    double sourceIn() const { return sourceIn_; }
    double sourceOut() const { return sourceOut_; }
    double sourceDuration() const { return sourceOut_ - sourceIn_; }
    void setSourceIn(double v) { sourceIn_ = v; }
    void setSourceOut(double v) { sourceOut_ = v; }

    // Timeline placement (seconds on the timeline)
    double timelineStart() const { return timelineStart_; }
    void setTimelineStart(double v) { timelineStart_ = v; }
    double duration() const { return sourceOut_ - sourceIn_; }

    // Transform (video/image)
    double posX() const { return posX_; }
    double posY() const { return posY_; }
    double scale() const { return scale_; }
    double opacity() const { return opacity_; }
    void setPosX(double v) { posX_ = v; emit changed(); }
    void setPosY(double v) { posY_ = v; emit changed(); }
    void setScale(double v) { scale_ = v; emit changed(); }
    void setOpacity(double v) { opacity_ = v; emit changed(); }

    // Audio
    double volume() const { return volume_; }
    double pan() const { return pan_; }
    void setVolume(double v) { volume_ = v; emit changed(); }
    void setPan(double v) { pan_ = v; emit changed(); }

    // Cached thumbnail / waveform image
    const QImage& thumbnail() const { return thumbnail_; }
    void setThumbnail(const QImage& img) { thumbnail_ = img; }

signals:
    void changed();

private:
    QString id_;
    QString sourcePath_;
    MediaType type_ = MediaType::Video;

    double sourceIn_      = 0.0;
    double sourceOut_     = 0.0;
    double timelineStart_ = 0.0;

    double posX_     = 0.0;
    double posY_     = 0.0;
    double scale_    = 1.0;
    double opacity_  = 1.0;

    double volume_   = 1.0;
    double pan_      = 0.0;

    QImage thumbnail_;
};

} // namespace ve
