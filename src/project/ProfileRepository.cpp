#include "project/ProfileRepository.h"

namespace ve {

ProfileRepository& ProfileRepository::self() {
    static ProfileRepository instance;
    static bool loaded = false;
    if (!loaded) {
        instance.loadBuiltin();
        loaded = true;
    }
    return instance;
}

void ProfileRepository::loadBuiltin() {
    m_profiles.clear();
    for (const QString& d : {
        "1080p25", "1080p30", "1080p50",
        "720p25",  "720p30",  "720p50",
        "4k25",    "4k60",
        "square",  "vertical"
    }) {
        m_profiles.append(Profile::fromDescription(d));
    }
}

Profile ProfileRepository::byDescription(const QString& desc) const {
    for (const Profile& p : m_profiles) {
        if (p.description == desc) return p;
    }
    return m_profiles.value(0);
}

} // namespace ve
