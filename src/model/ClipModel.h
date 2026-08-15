/*
 * VideoEditor - ClipModel.h
 * Timeline clip instance, referencing a BinClip by id.
 *
 * Adapted from Kdenlive's src/timeline2/model/clipmodel.hpp.
 *
 * A ClipModel is an instance of a BinClip placed on the timeline. Multiple
 * ClipModels can reference the same BinClip (e.g. when the same source is
 * used at different points in the timeline). The ClipModel only stores:
 *   - which BinClip it references (binClipId)
 *   - position on the timeline (frames, inherited from MoveableItem)
 *   - source in/out (frames, for trimming)
 *   - which track it's on
 *   - its state (VideoOnly / AudioOnly / Disabled)
 *   - speed (1.0 default)
 *
 * Per-clip effects live in an EffectStackModel owned by this clip.
 */
#pragma once

#include "MoveableItem.h"
#include "../definitions.h"
#include <memory>
#include <QString>
#include <QObject>

namespace ve {

class TimelineModel;
class BinClip;
class EffectStackModel;

class ClipModel : public QObject, public MoveableItem {
    Q_OBJECT
public:
    static ObjectId construct(TimelineModel* parent, const QString& binClipId,
                              ObjectId id, ClipState state = ClipState::Unknown,
                              double speed = 1.0);

    const QString& binClipId() const { return m_binClipId; }
    ClipState state() const { return m_state; }
    void setState(ClipState s) { m_state = s; emit changed(); }

    double speed() const { return m_speed; }
    void   setSpeed(double s) { m_speed = s; emit changed(); }

    /// Underlying BinClip (looked up from the bin).
    std::shared_ptr<BinClip> binClip() const;

    /// Effect stack for this clip.
    std::shared_ptr<EffectStackModel> effectStack() const { return m_effectStack; }

    /// Compute a thumbnail for display on the timeline clip item.
    QImage thumbnail() const;

    /// Frame-accurate duration considering speed.
    int getPlaytime() const override;

signals:
    void changed();

public:
    ClipModel(TimelineModel* parent, const QString& binClipId, ObjectId id,
              ClipState state, double speed);

private:
    QString m_binClipId;
    ClipState m_state = ClipState::Unknown;
    double    m_speed = 1.0;
    std::shared_ptr<EffectStackModel> m_effectStack;
};

} // namespace ve
