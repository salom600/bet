#include "utils/JsonSerializer.h"
#include "core/Project.h"
#include "core/Timeline.h"
#include "core/Track.h"
#include "core/Clip.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

namespace ve {

QJsonObject JsonSerializer::clipToJson(const Clip* c) {
    QJsonObject o;
    o["id"]            = c->id();
    o["sourcePath"]    = c->sourcePath();
    o["type"]          = static_cast<int>(c->type());
    o["sourceIn"]      = c->sourceIn();
    o["sourceOut"]     = c->sourceOut();
    o["timelineStart"] = c->timelineStart();
    o["posX"]          = c->posX();
    o["posY"]          = c->posY();
    o["scale"]         = c->scale();
    o["opacity"]       = c->opacity();
    o["volume"]        = c->volume();
    o["pan"]           = c->pan();
    return o;
}

Clip* JsonSerializer::clipFromJson(const QJsonObject& o) {
    auto* c = new Clip;
    c->setId(o["id"].toString());
    c->setSourcePath(o["sourcePath"].toString());
    c->setType(static_cast<MediaType>(o["type"].toInt()));
    c->setSourceIn(o["sourceIn"].toDouble());
    c->setSourceOut(o["sourceOut"].toDouble());
    c->setTimelineStart(o["timelineStart"].toDouble());
    c->setPosX(o["posX"].toDouble());
    c->setPosY(o["posY"].toDouble());
    c->setScale(o["scale"].toDouble());
    c->setOpacity(o["opacity"].toDouble());
    c->setVolume(o["volume"].toDouble());
    c->setPan(o["pan"].toDouble());
    return c;
}

QJsonObject JsonSerializer::trackToJson(const Track* t) {
    QJsonObject o;
    o["name"]    = t->name();
    o["kind"]    = static_cast<int>(t->kind());
    o["muted"]   = t->isMuted();
    o["locked"]  = t->isLocked();
    o["visible"] = t->isVisible();
    QJsonArray clips;
    for (const Clip* c : t->clips()) clips.append(clipToJson(c));
    o["clips"] = clips;
    return o;
}

Track* JsonSerializer::trackFromJson(const QJsonObject& o) {
    auto kind = static_cast<Track::Kind>(o["kind"].toInt());
    auto* t = new Track(kind);
    t->setName(o["name"].toString());
    t->setMuted(o["muted"].toBool());
    t->setLocked(o["locked"].toBool());
    t->setVisible(o["visible"].toBool());
    const QJsonArray clips = o["clips"].toArray();
    for (const auto& v : clips) {
        Clip* c = clipFromJson(v.toObject());
        t->addClip(c);
    }
    return t;
}

QJsonObject JsonSerializer::timelineToJson(const Timeline* t) {
    QJsonObject o;
    o["pps"] = t->pixelsPerSecond();
    QJsonArray tracks;
    for (const Track* tr : t->tracks()) tracks.append(trackToJson(tr));
    o["tracks"] = tracks;
    return o;
}

void JsonSerializer::timelineFromJson(Timeline* t, const QJsonObject& o) {
    // Clear existing tracks
    const auto tracksCopy = t->tracks();
    for (Track* tr : tracksCopy) t->removeTrack(tr);

    const QJsonArray tracks = o["tracks"].toArray();
    for (const auto& v : tracks) {
        Track* tr = trackFromJson(v.toObject());
        tr->setParent(t);
        t->tracks().append(tr);
        emit t->trackAdded(tr);
    }
    t->setPixelsPerSecond(o["pps"].toDouble(50.0));
    emit t->structureChanged();
}

bool JsonSerializer::save(Project* project, const QString& path, QString* err) {
    QJsonObject root;
    root["version"]     = 1;
    root["filePath"]    = project->filePath();
    root["exportWidth"]  = project->exportWidth();
    root["exportHeight"] = project->exportHeight();
    root["exportFps"]    = project->exportFps();
    root["exportBitrateKbps"] = project->exportBitrateKbps();
    root["exportFormat"] = project->exportFormat();
    root["timeline"]    = timelineToJson(project->timeline());

    QJsonDocument doc(root);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (err) *err = QStringLiteral("Cannot write %1: %2").arg(path, f.errorString());
        return false;
    }
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

bool JsonSerializer::load(Project* project, const QString& path, QString* err) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (err) *err = QStringLiteral("Cannot read %1: %2").arg(path, f.errorString());
        return false;
    }
    const QByteArray data = f.readAll();
    f.close();

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        if (err) *err = QStringLiteral("Parse error: %1").arg(parseErr.errorString());
        return false;
    }
    const QJsonObject root = doc.object();

    project->setFilePath(root["filePath"].toString(path));
    project->setExportWidth(root["exportWidth"].toInt(1920));
    project->setExportHeight(root["exportHeight"].toInt(1080));
    project->setExportFps(root["exportFps"].toInt(30));
    project->setExportBitrateKbps(root["exportBitrateKbps"].toInt(8000));
    project->setExportFormat(root["exportFormat"].toString("mp4"));

    timelineFromJson(project->timeline(), root["timeline"].toObject());
    return true;
}

} // namespace ve
