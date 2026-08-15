#include "project/Project.h"
#include "project/ProfileRepository.h"

namespace ve {

Project::Project(QObject* parent)
    : QObject(parent)
    , m_bin(std::make_shared<BinModel>())
    , m_timeline(std::make_shared<TimelineModel>(m_bin))
    , m_profile(ProfileRepository::self().defaultProfile())
{
    m_timeline->setFps(m_profile.fps());
    GenTime::setFps(m_profile.fps());
}

} // namespace ve
