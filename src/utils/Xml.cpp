#include "utils/Xml.h"

namespace ve {
namespace Xml {

QString getXmlProperty(const QDomElement& element, const QString& propertyName,
                       const QString& defaultReturn) {
    QDomNodeList children = element.childNodes();
    for (int i = 0; i < children.size(); ++i) {
        QDomElement child = children.at(i).toElement();
        if (child.tagName() == "property" && child.attribute("name") == propertyName) {
            return child.text();
        }
    }
    return defaultReturn;
}

bool hasXmlProperty(const QDomElement& element, const QString& propertyName) {
    return !getXmlProperty(element, propertyName, QStringLiteral("\x01_sentinel")).isEmpty();
}

void setXmlProperty(QDomElement element, const QString& name, const QString& value) {
    QDomDocument doc = element.ownerDocument();
    // Try to update existing
    QDomNodeList children = element.childNodes();
    for (int i = 0; i < children.size(); ++i) {
        QDomElement child = children.at(i).toElement();
        if (child.tagName() == "property" && child.attribute("name") == name) {
            // Clear text and set new
            QDomNodeList textNodes = child.childNodes();
            for (int j = 0; j < textNodes.size(); ++j) child.removeChild(textNodes.at(j));
            child.appendChild(doc.createTextNode(value));
            return;
        }
    }
    // Create new
    QDomElement p = doc.createElement("property");
    p.setAttribute("name", name);
    p.appendChild(doc.createTextNode(value));
    element.appendChild(p);
}

void removeXmlProperty(QDomElement element, const QString& name) {
    QDomNodeList children = element.childNodes();
    for (int i = 0; i < children.size(); ++i) {
        QDomElement child = children.at(i).toElement();
        if (child.tagName() == "property" && child.attribute("name") == name) {
            element.removeChild(children.at(i));
            return;
        }
    }
}

} // namespace Xml
} // namespace ve
