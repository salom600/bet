#pragma once

#include <QObject>
#include <QList>
#include <memory>
#include "core/Clip.h"

namespace ve {

/// A timeline track that holds a list of clips.
class Track : public QObject {
    Q_OBJECT
public:
    enum class Kind { Video, Image, Audio };

    explicit Track(Kind kind, QObject* parent = nullptr);

    Kind kind() const { return kind_; }
    void setKind(Kind k) { kind_ = k; }

    QString name() const { return name_; }
    void setName(const QString& n) { name_ = n; emit changed(); }

    bool isMuted() const { return muted_; }
    bool isLocked() const { return locked_; }
    bool isVisible() const { return visible_; }
    void setMuted(bool v)    { muted_   = v; emit changed(); }
    void setLocked(bool v)   { locked_  = v; emit changed(); }
    void setVisible(bool v)  { visible_ = v; emit changed(); }

    const QList<Clip*>& clips() const { return clips_; }
    QList<Clip*>& clips() { return clips_; }

    void addClip(Clip* clip);
    void removeClip(Clip* clip);
    void insertClip(int index, Clip* clip);

signals:
    void clipAdded(Clip* clip);
    void clipRemoved(Clip* clip);
    void changed();

private:
    Kind kind_;
    QString name_;
    bool muted_   = false;
    bool locked_  = false;
    bool visible_ = true;
    QList<Clip*> clips_;
};

} // namespace ve
