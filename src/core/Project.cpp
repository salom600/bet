#include "core/Project.h"

namespace ve {

Project::Project(QObject* parent)
    : QObject(parent)
    , timeline_(new Timeline(this))
{
}

} // namespace ve
