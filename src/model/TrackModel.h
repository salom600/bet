/*
 * VideoEditor - TrackModel.h
 * One timeline track.
 *
 * Adapted from Kdenlive's src/timeline2/model/trackmodel.hpp.
 *
 * A TrackModel holds:
 *   - a sorted list of clip IDs (sorted by position)
 *   - track properties: name, type, muted, locked, visible
 *   - a reference to the owning TimelineModel
 *
 * Adding/removing/moving clips on a track is done via the TimelineModel's
 * request* methods (which generate undo/redo). The TrackModel only stores
 * the result.
 */
#pragma once

#include "../definitions.h"
#include <QString>
#include <QList>
#include <QObject>
#include <memory>

namespace ve {

class TimelineModel;
class ClipModel;

class TrackModel : public QObject {
    Q_OBJECT
public:
    TrackModel(TimelineModel* parent, ObjectId id, TrackType type);

    ObjectId id() const { return m_id; }
    TrackType type() const { return m_type; }
    void setType(TrackType t) { m_type = t; emit changed(); }

    QString name() const { return m_name; }
    void setName(const QString& n) { m_name = n; emit changed(); }

    bool isMuted()  const { return m_muted; }
    bool isLocked() const { return m_locked; }
    bool isVisible() const { return m_visible; }
    void setMuted(bool v);
    void setLocked(bool v);
    void setVisible(bool v);

    const QList<ObjectId>& clipIds() const { return m_clipIds; }
    void addClip(ObjectId clipId);
    void removeClip(ObjectId clipId);

    /// Sorted by clip position (ascending).
    QList<ObjectId> clipsSorted() const;

signals:
    void changed();
    void clipAdded(ObjectId clipId);
    void clipRemoved(ObjectId clipId);

private:
    TimelineModel* m_parent;
    ObjectId m_id;
    TrackType m_type;
    QString m_name;
    bool m_muted   = false;
    bool m_locked  = false;
    bool m_visible = true;
    QList<ObjectId> m_clipIds; // not necessarily sorted; sort on demand
};

} // namespace ve
