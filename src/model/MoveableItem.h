/*
 * VideoEditor - MoveableItem.h
 * Base class for clips and compositions on the timeline.
 *
 * Adapted from Kdenlive's src/timeline2/model/moveableItem.hpp.
 *
 * Provides: id, position, in/out, current track id, selection state.
 * Subclasses (ClipModel, CompositionModel) supply the underlying service
 * reference and the duration.
 */
#pragma once

#include "../definitions.h"
#include "../utils/GenTime.h"

namespace ve {

class TimelineModel;

class MoveableItem {
public:
    MoveableItem(TimelineModel* parent, ObjectId id)
        : m_parent(parent), m_id(id) {}

    virtual ~MoveableItem() = default;

    ObjectId getId() const { return m_id; }

    /// Position on the timeline in frames.
    int getPosition() const { return m_position; }
    void setPosition(int p) { m_position = p; }

    /// In/out points (in source frames) for trimming.
    int getIn()  const { return m_in; }
    int getOut() const { return m_out; }
    void setIn(int v)  { m_in = v; }
    void setOut(int v) { m_out = v; }

    /// Length on the timeline.
    virtual int getPlaytime() const { return m_out - m_in; }

    /// Currently inserted track id, or INVALID_ID.
    ObjectId getCurrentTrackId() const { return m_trackId; }
    void setCurrentTrackId(ObjectId tid) { m_trackId = tid; }

    bool isSelected() const { return m_selected; }
    void setSelected(bool s) { m_selected = s; }

protected:
    TimelineModel* m_parent;
    ObjectId m_id;
    ObjectId m_trackId = INVALID_ID;
    int m_position = 0;
    int m_in  = 0;
    int m_out = 0;
    bool m_selected = false;
};

} // namespace ve
