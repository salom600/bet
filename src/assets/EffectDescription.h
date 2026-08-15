/*
 * VideoEditor - EffectDescription.h
 * Describes an effect type: id, name, type, parameters.
 *
 * Adapted from Kdenlive's assets/model/assetparametermodel.hpp.
 *
 * An EffectDescription is loaded from a JSON manifest at startup. It
 * describes what parameters the effect has (name, type, range, default)
 * so the UI can build an editor dynamically.
 *
 * The actual effect implementation is provided by the media backend.
 * For the basics, we ship a small set of built-in effects (transform,
 * fade-in, fade-out, volume, blur) implemented as CPU filters applied
 * during frame extraction.
 */
#pragma once

#include "../definitions.h"
#include <QString>
#include <QList>
#include <QVariant>

namespace ve {

struct EffectParameter {
    enum Type {
        Double, Int, Bool, Color, String, List
    };
    QString name;
    QString displayName;
    Type    type = Double;
    double  minVal   = 0.0;
    double  maxVal   = 1.0;
    double  defaultVal = 0.0;
    QStringList listValues;  // for List type
};

class EffectDescription {
public:
    QString id;
    QString name;
    QString description;
    enum class EffectType { Video, Audio };
    EffectType type = EffectType::Video;

    QList<EffectParameter> parameters;

    /// Get the default parameter values as a name->value map.
    QMap<QString, QVariant> defaultParameters() const;

    /// Built-in effect catalog (loaded at startup).
    static QList<EffectDescription> builtinEffects();
};

} // namespace ve
