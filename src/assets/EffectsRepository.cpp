#include "assets/EffectsRepository.h"

namespace ve {

EffectsRepository& EffectsRepository::self() {
    static EffectsRepository instance;
    static bool loaded = false;
    if (!loaded) {
        instance.loadBuiltin();
        loaded = true;
    }
    return instance;
}

void EffectsRepository::loadBuiltin() {
    for (const auto& e : EffectDescription::builtinEffects()) {
        m_effects.insert(e.id, e);
    }
}

EffectDescription EffectsRepository::byId(const QString& id) const {
    return m_effects.value(id);
}

} // namespace ve
