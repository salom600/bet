/*
 * VideoEditor - ProfileRepository.h
 * Registry of built-in render profiles.
 * Adapted from Kdenlive's src/profiles/profilerepository.hpp.
 *
 * On startup, ProfileRepository loads a small set of common profiles. The
 * user can select one when creating a new project. Custom profiles can be
 * added later via JSON manifests in resources/profiles/.
 */
#pragma once

#include "model/Profile.h"
#include <QList>
#include <QString>
#include <memory>

namespace ve {

class ProfileRepository {
public:
    static ProfileRepository& self();

    void loadBuiltin();

    QList<Profile> profiles() const { return m_profiles; }
    Profile defaultProfile() const { return m_profiles.value(0); }
    Profile byDescription(const QString& desc) const;

private:
    ProfileRepository() = default;
    QList<Profile> m_profiles;
};

} // namespace ve
