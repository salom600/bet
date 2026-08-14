/*
 * VideoEditor - Project.h
 * Top-level project: owns Bin + Timeline + Profile + file path.
 *
 * Adapted from Kdenlive's src/doc/kdenlivedoc.h.
 */
#pragma once

#include "model/Profile.h"
#include "model/BinModel.h"
#include "model/TimelineModel.h"
#include <QObject>
#include <QString>
#include <QUndoStack>
#include <memory>

namespace ve {

class Project : public QObject {
    Q_OBJECT
public:
    explicit Project(QObject* parent = nullptr);

    std::shared_ptr<BinModel> bin() const { return m_bin; }
    std::shared_ptr<TimelineModel> timeline() const { return m_timeline; }

    const Profile& profile() const { return m_profile; }
    void setProfile(const Profile& p) { m_profile = p; m_timeline->setFps(p.fps()); emit dirty(); }

    const QString& filePath() const { return m_filePath; }
    void setFilePath(const QString& p) { m_filePath = p; emit dirty(); }

    bool isDirty() const { return m_dirty; }
    void setDirty(bool v) { m_dirty = v; emit dirty(); }

    QUndoStack* undoStack() { return &m_undoStack; }

signals:
    void dirty();

private:
    std::shared_ptr<BinModel>      m_bin;
    std::shared_ptr<TimelineModel> m_timeline;
    Profile m_profile;
    QString m_filePath;
    bool    m_dirty = false;
    QUndoStack m_undoStack;
};

} // namespace ve
