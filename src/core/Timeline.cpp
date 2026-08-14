#include "core/Timeline.h"
#include <algorithm>
#include <cmath>

namespace ve {

Timeline::Timeline(QObject* parent)
    : QObject(parent)
{
    // Pre-seed the timeline with the minimum required tracks:
    // 2 video, 2 image, 2 audio (per spec)
    addTrack(Track::Kind::Video);
    addTrack(Track::Kind::Video);
    addTrack(Track::Kind::Image);
    addTrack(Track::Kind::Image);
    addTrack(Track::Kind::Audio);
    addTrack(Track::Kind::Audio);
}

Track* Timeline::addTrack(Track::Kind kind) {
    auto* t = new Track(kind, this);
    tracks_.append(t);
    emit trackAdded(t);
    emit structureChanged();
    return t;
}

void Timeline::removeTrack(Track* track) {
    if (tracks_.removeOne(track)) {
        track->deleteLater();
        emit trackRemoved(track);
        emit structureChanged();
    }
}

double Timeline::duration() const {
    double maxEnd = 0.0;
    for (Track* t : tracks_) {
        for (Clip* c : t->clips()) {
            double end = c->timelineStart() + c->duration();
            if (end > maxEnd) maxEnd = end;
        }
    }
    return maxEnd;
}

double Timeline::snap(double t, double tolerance, double playhead) const {
    // Candidate snap points: playhead + every clip edge on every track
    std::vector<double> candidates;
    candidates.push_back(playhead);
    for (Track* tr : tracks_) {
        for (Clip* c : tr->clips()) {
            candidates.push_back(c->timelineStart());
            candidates.push_back(c->timelineStart() + c->duration());
        }
    }
    double best = t;
    double bestDelta = tolerance;
    for (double c : candidates) {
        double d = std::fabs(c - t);
        if (d < bestDelta) {
            bestDelta = d;
            best = c;
        }
    }
    return best;
}

Clip* Timeline::clipAt(Track* track, double time) const {
    if (!track) return nullptr;
    for (Clip* c : track->clips()) {
        double start = c->timelineStart();
        double end   = start + c->duration();
        if (time >= start && time < end) return c;
    }
    return nullptr;
}

QList<Clip*> Timeline::clipsInRange(double start, double end) const {
    QList<Clip*> out;
    for (Track* t : tracks_) {
        for (Clip* c : t->clips()) {
            double cs = c->timelineStart();
            double ce = cs + c->duration();
            if (ce > start && cs < end) out.append(c);
        }
    }
    return out;
}

} // namespace ve
