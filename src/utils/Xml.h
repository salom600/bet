/*
 * VideoEditor - Xml.h
 * QDom helper namespace.
 *
 * Adapted from Kdenlive's src/xml/xml.hpp.
 */
#pragma once

#include <QDomElement>
#include <QString>
#include <QMap>

namespace ve {
namespace Xml {

QString getXmlProperty(const QDomElement& element, const QString& propertyName,
                       const QString& defaultReturn = QString());
bool    hasXmlProperty(const QDomElement& element, const QString& propertyName);
void    setXmlProperty(QDomElement element, const QString& name, const QString& value);
void    removeXmlProperty(QDomElement element, const QString& name);

} // namespace Xml
} // namespace ve
