#include "model/SnapModel.h"
#include <algorithm>
#include <cmath>

namespace ve {

void SnapModel::addPoint(int position) {
    m_snaps[position]++;
}

void SnapModel::removePoint(int position) {
    auto it = m_snaps.find(position);
    if (it == m_snaps.end()) return;
    if (--(it->second) <= 0) m_snaps.erase(it);
}

int SnapModel::getClosestPoint(int position, int maxDist) const {
    if (m_snaps.empty()) return -1;
    // lower_bound gives first element >= position
    auto it = m_snaps.lower_bound(position);
    int best = -1;
    int bestDist = maxDist + 1;
    if (it != m_snaps.end()) {
        int d = std::abs(it->first - position);
        if (d <= maxDist && d < bestDist) { best = it->first; bestDist = d; }
    }
    if (it != m_snaps.begin()) {
        --it;
        int d = std::abs(it->first - position);
        if (d <= maxDist && d < bestDist) { best = it->first; bestDist = d; }
    }
    return best;
}

int SnapModel::getNextPoint(int position) const {
    auto it = m_snaps.upper_bound(position);
    return it == m_snaps.end() ? position : it->first;
}

int SnapModel::getPreviousPoint(int position) const {
    if (m_snaps.empty()) return 0;
    auto it = m_snaps.lower_bound(position);
    if (it == m_snaps.begin()) return 0;
    --it;
    return it->first;
}

} // namespace ve
