/*
 * VideoEditor - EffectStackModel.h
 * Ordered list of effects on a clip or track.
 *
 * Adapted from Kdenlive's src/effects/effectstack/model/effectstackmodel.hpp.
 *
 * The stack owns a list of EffectItemModel instances. Effects are applied
 * in order during rendering. The stack supports:
 *   - appendEffect(effectId)
 *   - removeEffect(item)
 *   - moveEffect(fromRow, toRow)
 *   - setEffectStackEnabled(bool)
 *
 * All mutations generate undo/redo lambdas (Kdenlive-style).
 */
#pragma once

#include "../definitions.h"
#include "../undohelper.h"
#include <QObject>
#include <QList>
#include <QtXml/QDomElement>
#include <memory>

namespace ve {

class EffectItemModel;

class EffectStackModel : public QObject, public std::enable_shared_from_this<EffectStackModel> {
    Q_OBJECT
public:
    static std::shared_ptr<EffectStackModel> construct(ObjectId ownerId);

    /// Append an effect of the given type.
    std::shared_ptr<EffectItemModel> appendEffect(const QString& effectId);

    /// Remove an effect.
    bool removeEffect(std::shared_ptr<EffectItemModel> item);

    /// Move an effect to a new row.
    bool moveEffect(std::shared_ptr<EffectItemModel> item, int destRow);

    /// Look up an effect by row.
    std::shared_ptr<EffectItemModel> at(int row) const { return m_effects.value(row); }

    int rowCount() const { return m_effects.size(); }
    const QList<std::shared_ptr<EffectItemModel>>& effects() const { return m_effects; }

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool e) { m_enabled = e; emit changed(); }

    ObjectId ownerId() const { return m_ownerId; }

    /// Serialize to XML.
    void toXml(QDomElement& parent) const;
    void fromXml(const QDomElement& parent);

signals:
    void changed();
    void effectAdded(std::shared_ptr<EffectItemModel> item);
    void effectRemoved(std::shared_ptr<EffectItemModel> item);

private:
    EffectStackModel(ObjectId ownerId) : m_ownerId(ownerId) {}

    ObjectId m_ownerId;
    QList<std::shared_ptr<EffectItemModel>> m_effects;
    bool m_enabled = true;
};

} // namespace ve
