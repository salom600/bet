#pragma once

#include <QObject>
#include <QList>
#include "core/Track.h"

namespace ve {

/// The timeline owns a list of tracks.
class Timeline : public QObject {
    Q_OBJECT
public:
    explicit Timeline(QObject* parent = nullptr);

    const QList<Track*>& tracks() const { return tracks_; }
    QList<Track*>& tracks() { return tracks_; }

    Track* addTrack(Track::Kind kind);
    void removeTrack(Track* track);

    /// Total duration in seconds (max end time across all clips).
    double duration() const;

    /// Snap a time value to the nearest clip edge / playhead within tolerance.
    double snap(double t, double tolerance, double playhead) const;

    /// Find clip at given timeline time on given track (or nullptr).
    Clip* clipAt(Track* track, double time) const;

    /// Find all clips that overlap a time range across all tracks.
    QList<Clip*> clipsInRange(double start, double end) const;

    /// Pixels per second (horizontal zoom). Used by UI widgets.
    double pixelsPerSecond() const { return pps_; }
    void setPixelsPerSecond(double v) { pps_ = v; emit layoutChanged(); }

signals:
    void trackAdded(Track* t);
    void trackRemoved(Track* t);
    void structureChanged();
    void layoutChanged();

private:
    QList<Track*> tracks_;
    double pps_ = 50.0; // default 50 px / s
};

} // namespace ve
