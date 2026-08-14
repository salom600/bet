/*
 * VideoEditor - EffectItemModel.h
 * One effect instance with its current parameter values.
 *
 * Adapted from Kdenlive's src/effects/effectstack/model/effectitemmodel.hpp.
 */
#pragma once

#include "EffectDescription.h"
#include "../definitions.h"
#include <QObject>
#include <QMap>
#include <QVariant>
#include <memory>

namespace ve {

class EffectStackModel;

class EffectItemModel : public QObject {
    Q_OBJECT
public:
    static std::shared_ptr<EffectItemModel> create(const QString& effectId,
                                                    std::shared_ptr<EffectStackModel> stack);

    const QString& effectId() const { return m_effectId; }
    QString name() const { return m_description.name; }
    const EffectDescription& description() const { return m_description; }

    QVariant parameter(const QString& name) const;
    void setParameter(const QString& name, QVariant value);
    QMap<QString, QVariant> parameters() const { return m_params; }

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool e) { m_enabled = e; emit changed(); }

signals:
    void changed();

private:
    EffectItemModel(const EffectDescription& d, std::shared_ptr<EffectStackModel> stack);

    QString m_effectId;
    EffectDescription m_description;
    QMap<QString, QVariant> m_params;
    bool m_enabled = true;
};

} // namespace ve
