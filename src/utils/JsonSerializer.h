#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace ve {

class Project;
class Timeline;
class Track;
class Clip;

/// Serialize / deserialize a Project to/from JSON (.veproj) format.
class JsonSerializer {
public:
    static bool save(Project* project, const QString& path, QString* err = nullptr);
    static bool load(Project* project, const QString& path, QString* err = nullptr);

    static QJsonObject clipToJson(const Clip* c);
    static Clip* clipFromJson(const QJsonObject& o);

    static QJsonObject trackToJson(const Track* t);
    static Track* trackFromJson(const QJsonObject& o);

    static QJsonObject timelineToJson(const Timeline* t);
    static void timelineFromJson(Timeline* t, const QJsonObject& o);
};

} // namespace ve
