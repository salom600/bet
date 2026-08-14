/*
 * VideoEditor - ProjectSerializer.cpp
 */
#include "project/ProjectSerializer.h"
#include "project/Project.h"
#include "model/BinModel.h"
#include "model/BinClip.h"
#include "model/BinFolder.h"
#include "model/TimelineModel.h"
#include "model/TrackModel.h"
#include "model/ClipModel.h"
#include "model/Profile.h"
#include "definitions.h"

#include <QFile>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QTextStream>
#include <QDebug>

namespace ve {

namespace {

void serializeFolder(QDomDocument& doc, QDomElement& parent, const std::shared_ptr<BinFolder>& folder);

void serializeClip(QDomDocument& doc, QDomElement& parent, const std::shared_ptr<BinClip>& clip) {
    QDomElement e = doc.createElement("clip");
    e.setAttribute("id", clip->id());
    e.setAttribute("path", clip->sourcePath());
    e.setAttribute("type", static_cast<int>(clip->type()));
    e.setAttribute("duration", QString::number(clip->duration(), 'f', 3));
    e.setAttribute("name", clip->name());
    parent.appendChild(e);
}

void serializeFolder(QDomDocument& doc, QDomElement& parent, const std::shared_ptr<BinFolder>& folder) {
    QDomElement e = doc.createElement("folder");
    e.setAttribute("id", folder->id());
    e.setAttribute("name", folder->name());
    for (const auto& sub : folder->folders()) {
        serializeFolder(doc, e, sub);
    }
    for (const auto& clip : folder->clips()) {
        serializeClip(doc, e, clip);
    }
    parent.appendChild(e);
}

void serializeTrack(QDomDocument& doc, QDomElement& parent, const std::shared_ptr<TimelineModel>& tl,
                    const std::shared_ptr<TrackModel>& track) {
    QDomElement e = doc.createElement("track");
    e.setAttribute("id", track->id());
    e.setAttribute("type", static_cast<int>(track->type()));
    e.setAttribute("name", track->name());
    e.setAttribute("muted", track->isMuted() ? "1" : "0");
    e.setAttribute("locked", track->isLocked() ? "1" : "0");
    e.setAttribute("visible", track->isVisible() ? "1" : "0");
    for (ObjectId clipId : track->clipsSorted()) {
        auto clip = tl->clip(clipId);
        if (!clip) continue;
        QDomElement c = doc.createElement("clip");
        c.setAttribute("id", clip->getId());
        c.setAttribute("binClipId", clip->binClipId());
        c.setAttribute("position", clip->getPosition());
        c.setAttribute("in", clip->getIn());
        c.setAttribute("out", clip->getOut());
        c.setAttribute("state", static_cast<int>(clip->state()));
        e.appendChild(c);
    }
    parent.appendChild(e);
}

void serializeProfile(QDomDocument& doc, QDomElement& root, const Profile& p) {
    QDomElement e = doc.createElement("profile");
    e.setAttribute("width", p.width);
    e.setAttribute("height", p.height);
    e.setAttribute("frame_rate_num", p.fps_num);
    e.setAttribute("frame_rate_den", p.fps_den);
    e.setAttribute("sample_aspect_num", p.sar_num);
    e.setAttribute("sample_aspect_den", p.sar_den);
    e.setAttribute("display_aspect_num", p.dar_num);
    e.setAttribute("display_aspect_den", p.dar_den);
    e.setAttribute("colorspace", p.colorspace);
    e.setAttribute("progressive", p.progressive ? "1" : "0");
    e.setAttribute("description", p.description);
    root.appendChild(e);
}

} // namespace

bool ProjectSerializer::save(Project* project, const QString& path, QString* err) {
    QDomDocument doc;
    QDomElement root = doc.createElement("veproject");
    root.setAttribute("version", "1");
    root.setAttribute("producer", "main_bin");
    doc.appendChild(root);

    serializeProfile(doc, root, project->profile());

    QDomElement binEl = doc.createElement("bin");
    serializeFolder(doc, binEl, project->bin()->folder(project->bin()->rootFolderId()));
    root.appendChild(binEl);

    QDomElement tlEl = doc.createElement("timeline");
    for (ObjectId tid : project->timeline()->trackIds()) {
        serializeTrack(doc, tlEl, project->timeline(), project->timeline()->track(tid));
    }
    root.appendChild(tlEl);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (err) *err = QStringLiteral("Cannot write %1: %2").arg(path, f.errorString());
        return false;
    }
    QTextStream stream(&f);
    stream.setEncoding(QStringConverter::Utf8);
    stream << doc.toString(2);
    f.close();
    return true;
}

bool ProjectSerializer::load(Project* project, const QString& path, QString* err) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (err) *err = QStringLiteral("Cannot read %1: %2").arg(path, f.errorString());
        return false;
    }
    QByteArray data = f.readAll();
    f.close();

    QDomDocument doc;
    QString parseErr;
    int line = 0, col = 0;
    if (!doc.setContent(data, &parseErr, &line, &col)) {
        if (err) *err = QStringLiteral("Parse error at line %1 col %2: %3").arg(line).arg(col).arg(parseErr);
        return false;
    }

    QDomElement root = doc.documentElement();
    if (root.tagName() != "veproject") {
        if (err) *err = QStringLiteral("Not a veproject file (root tag: %1)").arg(root.tagName());
        return false;
    }

    // Profile
    QDomElement profileEl = root.firstChildElement("profile");
    Profile p;
    if (!profileEl.isNull()) {
        p.width       = profileEl.attribute("width", "1920").toInt();
        p.height      = profileEl.attribute("height", "1080").toInt();
        p.fps_num     = profileEl.attribute("frame_rate_num", "25").toInt();
        p.fps_den     = profileEl.attribute("frame_rate_den", "1").toInt();
        p.sar_num     = profileEl.attribute("sample_aspect_num", "1").toInt();
        p.sar_den     = profileEl.attribute("sample_aspect_den", "1").toInt();
        p.dar_num     = profileEl.attribute("display_aspect_num", "16").toInt();
        p.dar_den     = profileEl.attribute("display_aspect_den", "9").toInt();
        p.colorspace  = profileEl.attribute("colorspace", "709").toInt();
        p.progressive = profileEl.attribute("progressive", "1") == "1";
        p.description = profileEl.attribute("description", "HD 1080p 25 fps");
    }
    project->setProfile(p);

    // We don't replace the existing bin/timeline (they were created by the
    // Project ctor); instead we load items into them.
    // For simplicity, this loader clears and re-populates.
    auto bin = project->bin();
    auto tl = project->timeline();

    // Bin: walk <bin>/<folder> recursively
    QDomElement binEl = root.firstChildElement("bin");
    if (!binEl.isNull()) {
        // For each top-level clip, call bin->addClip(path, root)
        QDomNodeList clips = binEl.elementsByTagName("clip");
        for (int i = 0; i < clips.size(); ++i) {
            QDomElement c = clips.at(i).toElement();
            QString filePath = c.attribute("path");
            if (!filePath.isEmpty()) {
                bin->addClip(filePath, bin->rootFolderId());
            }
        }
    }

    // Timeline: walk <timeline>/<track> and re-insert clips
    QDomElement tlEl = root.firstChildElement("timeline");
    if (!tlEl.isNull()) {
        QDomNodeList tracks = tlEl.elementsByTagName("track");
        for (int i = 0; i < tracks.size(); ++i) {
            QDomElement tEl = tracks.at(i).toElement();
            ObjectId trackId = (i < tl->trackIds().size()) ? tl->trackIds().at(i) : ve::INVALID_ID;
            auto track = tl->track(trackId);
            if (!track) continue;
            track->setName(tEl.attribute("name", track->name()));
            track->setMuted(tEl.attribute("muted") == "1");
            track->setLocked(tEl.attribute("locked") == "1");
            track->setVisible(tEl.attribute("visible") != "0");

            QDomNodeList clips = tEl.elementsByTagName("clip");
            for (int j = 0; j < clips.size(); ++j) {
                QDomElement cEl = clips.at(j).toElement();
                QString binClipId = cEl.attribute("binClipId");
                int position = cEl.attribute("position", "0").toInt();
                int in = cEl.attribute("in", "0").toInt();
                int out = cEl.attribute("out", "-1").toInt();
                tl->requestClipInsertion(binClipId, trackId, position, in, out);
            }
        }
    }

    project->setFilePath(path);
    return true;
}

} // namespace ve
