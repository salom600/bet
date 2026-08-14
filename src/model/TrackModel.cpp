#include "model/TrackModel.h"
#include "model/TimelineModel.h"
#include "model/ClipModel.h"
#include <algorithm>

namespace ve {

TrackModel::TrackModel(TimelineModel* parent, ObjectId id, TrackType type)
    : QObject(parent)
    , m_parent(parent)
    , m_id(id)
    , m_type(type)
{
    switch (m_type) {
        case TrackType::Video: m_name = "Video"; break;
        case TrackType::Image: m_name = "Image"; break;
        case TrackType::Audio: m_name = "Audio"; break;
    }
}

void TrackModel::setMuted(bool v)  { m_muted  = v; emit changed(); }
void TrackModel::setLocked(bool v) { m_locked = v; emit changed(); }
void TrackModel::setVisible(bool v) { m_visible = v; emit changed(); }

void TrackModel::addClip(ObjectId clipId) {
    if (!m_clipIds.contains(clipId)) {
        m_clipIds.append(clipId);
        emit clipAdded(clipId);
        emit changed();
    }
}

void TrackModel::removeClip(ObjectId clipId) {
    if (m_clipIds.removeOne(clipId)) {
        emit clipRemoved(clipId);
        emit changed();
    }
}

QList<ObjectId> TrackModel::clipsSorted() const {
    QList<ObjectId> out = m_clipIds;
    if (!m_parent) return out;
    std::sort(out.begin(), out.end(), [this](ObjectId a, ObjectId b) {
        auto ca = m_parent->clip(a);
        auto cb = m_parent->clip(b);
        if (!ca || !cb) return false;
        return ca->getPosition() < cb->getPosition();
    });
    return out;
}

} // namespace ve
