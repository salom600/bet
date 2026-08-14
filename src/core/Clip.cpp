#include "core/Clip.h"

namespace ve {

Clip::Clip(QObject* parent)
    : QObject(parent)
    , id_(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

} // namespace ve
