#include "core/Track.h"

namespace ve {

Track::Track(Kind kind, QObject* parent)
    : QObject(parent)
    , kind_(kind)
{
    switch (kind_) {
        case Kind::Video: name_ = "Video"; break;
        case Kind::Image: name_ = "Image"; break;
        case Kind::Audio: name_ = "Audio"; break;
    }
}

void Track::addClip(Clip* clip) {
    clips_.append(clip);
    clip->setParent(this);
    emit clipAdded(clip);
    emit changed();
}

void Track::removeClip(Clip* clip) {
    if (clips_.removeOne(clip)) {
        emit clipRemoved(clip);
        emit changed();
    }
}

void Track::insertClip(int index, Clip* clip) {
    if (index < 0) index = 0;
    if (index > clips_.size()) index = clips_.size();
    clips_.insert(index, clip);
    clip->setParent(this);
    emit clipAdded(clip);
    emit changed();
}

} // namespace ve
