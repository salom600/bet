/*
 * VideoEditor - SnapModel.h
 * Sorted snap-point registry.
 *
 * Adapted from Kdenlive's src/timeline2/model/snapmodel.hpp.
 *
 * The SnapModel maintains an ordered map of (position -> refcount). Adding
 * the same position twice increments its refcount; removing decrements.
 * getClosestPoint / getNextPoint / getPreviousPoint return the nearest
 * snap point within a tolerance.
 *
 * The TimelineModel owns one SnapModel and registers every clip's start
 * and end with it. When clips move/resize, the old positions are removed
 * and new positions are added.
 */
#pragma once

#include <map>
#include <vector>

namespace ve {

class SnapModel {
public:
    void addPoint(int position);
    void removePoint(int position);

    /// Returns closest snap point to `position`, or -1 if none within maxDist.
    int getClosestPoint(int position, int maxDist = 8) const;

    /// Returns next snap point strictly after `position`, or `position` if none.
    int getNextPoint(int position) const;

    /// Returns previous snap point strictly before `position`, or 0 if none.
    int getPreviousPoint(int position) const;

    bool empty() const { return m_snaps.empty(); }
    size_t size() const { return m_snaps.size(); }

    // For testing
    std::map<int, int> snaps() const { return m_snaps; }

private:
    // position -> refcount. std::map is ordered by key (unlike QHash).
    std::map<int, int> m_snaps;
};

} // namespace ve
