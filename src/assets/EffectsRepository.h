/*
 * VideoEditor - EffectsRepository.h
 * Registry of available effects.
 *
 * Adapted from Kdenlive's src/effects/effectsrepository.hpp.
 *
 * On startup, EffectsRepository loads the built-in effect catalog. Future
 * versions can extend this to scan for JSON manifests in resources/effects/
 * or to enumerate MLT/Frei0r plugins.
 */
#pragma once

#include "EffectDescription.h"
#include <QHash>
#include <QList>
#include <memory>

namespace ve {

class EffectsRepository {
public:
    static EffectsRepository& self();

    void loadBuiltin();

    QList<EffectDescription> all() const { return m_effects.values(); }
    EffectDescription byId(const QString& id) const;
    QStringList ids() const { return m_effects.keys(); }
    bool hasEffect(const QString& id) const { return m_effects.contains(id); }

private:
    EffectsRepository() = default;
    QHash<QString, EffectDescription> m_effects;
};

} // namespace ve
