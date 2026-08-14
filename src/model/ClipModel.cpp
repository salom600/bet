/*
 * VideoEditor - ClipModel.cpp
 */
#include "model/ClipModel.h"
#include "model/TimelineModel.h"
#include "model/BinClip.h"
#include "assets/EffectStackModel.h"

namespace ve {

ObjectId ClipModel::construct(TimelineModel* parent, const QString& binClipId,
                              ObjectId id, ClipState state, double speed) {
    auto* clip = new ClipModel(parent, binClipId, id, state, speed);
    Q_UNUSED(clip);
    return id;
}

ClipModel::ClipModel(TimelineModel* parent, const QString& binClipId,
                     ObjectId id, ClipState state, double speed)
    : QObject(parent)
    , MoveableItem(parent, id)
    , m_binClipId(binClipId)
    , m_state(state)
    , m_speed(speed)
    , m_effectStack(EffectStackModel::construct(id))
{
    // Auto-append a built-in 'transform' effect for video/image clips so the
    // user has position/scale/opacity controls out of the box.
    auto bc = binClip();
    if (bc) {
        if (bc->type() == ClipType::Video || bc->type() == ClipType::AV ||
            bc->type() == ClipType::Image) {
            m_effectStack->appendEffect("transform");
        } else if (bc->type() == ClipType::Audio) {
            m_effectStack->appendEffect("volume");
            m_effectStack->appendEffect("pan");
        }
    }
}

std::shared_ptr<BinClip> ClipModel::binClip() const {
    if (!m_parent) return nullptr;
    return m_parent->binClip(m_binClipId);
}

int ClipModel::getPlaytime() const {
    if (m_speed <= 0) return m_out - m_in;
    return static_cast<int>((m_out - m_in) / m_speed);
}

QImage ClipModel::thumbnail() const {
    auto bc = binClip();
    if (!bc) return QImage();
    return bc->thumbnail();
}

} // namespace ve
