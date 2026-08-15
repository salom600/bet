#include "assets/EffectItemModel.h"
#include "assets/EffectStackModel.h"
#include "assets/EffectsRepository.h"

namespace ve {

std::shared_ptr<EffectItemModel> EffectItemModel::create(const QString& effectId,
                                                          std::shared_ptr<EffectStackModel> stack) {
    auto& repo = EffectsRepository::self();
    if (!repo.hasEffect(effectId)) return nullptr;
    return std::shared_ptr<EffectItemModel>(new EffectItemModel(repo.byId(effectId), stack));
}

EffectItemModel::EffectItemModel(const EffectDescription& d, std::shared_ptr<EffectStackModel>)
    : QObject(nullptr)
    , m_effectId(d.id)
    , m_description(d)
    , m_params(d.defaultParameters())
{
}

QVariant EffectItemModel::parameter(const QString& name) const {
    return m_params.value(name);
}

void EffectItemModel::setParameter(const QString& name, QVariant value) {
    m_params.insert(name, value);
    emit changed();
}

} // namespace ve
