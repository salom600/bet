#include "model/BinFolder.h"
#include "model/BinClip.h"

namespace ve {

bool BinFolder::removeFolder(const QString& id) {
    for (int i = 0; i < folders_.size(); ++i) {
        if (folders_[i]->id() == id) { folders_.removeAt(i); return true; }
    }
    return false;
}

bool BinFolder::removeClip(const QString& id) {
    for (int i = 0; i < clips_.size(); ++i) {
        if (clips_[i]->id() == id) { clips_.removeAt(i); return true; }
    }
    return false;
}

} // namespace ve
