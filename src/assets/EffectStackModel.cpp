#include "assets/EffectStackModel.h"
#include "assets/EffectItemModel.h"

#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

namespace ve {

std::shared_ptr<EffectStackModel> EffectStackModel::construct(ObjectId ownerId) {
    return std::shared_ptr<EffectStackModel>(new EffectStackModel(ownerId));
}

std::shared_ptr<EffectItemModel> EffectStackModel::appendEffect(const QString& effectId) {
    auto item = EffectItemModel::create(effectId, shared_from_this());
    if (!item) return nullptr;
    m_effects.append(item);
    emit effectAdded(item);
    emit changed();
    return item;
}

bool EffectStackModel::removeEffect(std::shared_ptr<EffectItemModel> item) {
    if (!m_effects.removeOne(item)) return false;
    emit effectRemoved(item);
    emit changed();
    return true;
}

bool EffectStackModel::moveEffect(std::shared_ptr<EffectItemModel> item, int destRow) {
    int srcRow = m_effects.indexOf(item);
    if (srcRow < 0 || destRow < 0 || destRow >= m_effects.size()) return false;
    if (srcRow == destRow) return true;
    m_effects.move(srcRow, destRow);
    emit changed();
    return true;
}

void EffectStackModel::toXml(QDomElement& parent) const {
    QDomDocument doc = parent.ownerDocument();
    for (const auto& item : m_effects) {
        QDomElement e = doc.createElement("effect");
        e.setAttribute("id", item->effectId());
        e.setAttribute("enabled", item->isEnabled() ? "1" : "0");
        for (auto it = item->parameters().constBegin(); it != item->parameters().constEnd(); ++it) {
            QDomElement p = doc.createElement("param");
            p.setAttribute("name", it.key());
            p.appendChild(doc.createTextNode(it.value().toString()));
            e.appendChild(p);
        }
        parent.appendChild(e);
    }
}

void EffectStackModel::fromXml(const QDomElement& parent) {
    m_effects.clear();
    QDomNodeList effects = parent.elementsByTagName("effect");
    for (int i = 0; i < effects.size(); ++i) {
        QDomElement e = effects.at(i).toElement();
        QString id = e.attribute("id");
        auto item = appendEffect(id);
        if (!item) continue;
        item->setEnabled(e.attribute("enabled", "1") == "1");
        QDomNodeList params = e.elementsByTagName("param");
        for (int j = 0; j < params.size(); ++j) {
            QDomElement p = params.at(j).toElement();
            item->setParameter(p.attribute("name"), p.text());
        }
    }
}

} // namespace ve
