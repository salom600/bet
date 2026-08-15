#include "assets/EffectDescription.h"

namespace ve {

QMap<QString, QVariant> EffectDescription::defaultParameters() const {
    QMap<QString, QVariant> out;
    for (const auto& p : parameters) {
        out.insert(p.name, p.defaultVal);
    }
    return out;
}

QList<EffectDescription> EffectDescription::builtinEffects() {
    QList<EffectDescription> effects;

    // --- Transform (video) ---
    {
        EffectDescription e;
        e.id = "transform";
        e.name = "Transform";
        e.description = "Position, scale, opacity";
        e.type = EffectDescription::EffectType::Video;
        e.parameters.append({"posX",     "Position X", EffectParameter::Double, -10000, 10000, 0.0, {}});
        e.parameters.append({"posY",     "Position Y", EffectParameter::Double, -10000, 10000, 0.0, {}});
        e.parameters.append({"scale",    "Scale",      EffectParameter::Double, 0.01,   10.0,  1.0, {}});
        e.parameters.append({"opacity",  "Opacity",    EffectParameter::Double, 0.0,    1.0,   1.0, {}});
        effects.append(e);
    }

    // --- Fade in (video) ---
    {
        EffectDescription e;
        e.id = "fade_in";
        e.name = "Fade In";
        e.description = "Fade opacity from 0 to 1 over N frames";
        e.type = EffectDescription::EffectType::Video;
        e.parameters.append({"duration", "Duration (frames)", EffectParameter::Int, 1, 1000, 30, {}});
        effects.append(e);
    }

    // --- Fade out (video) ---
    {
        EffectDescription e;
        e.id = "fade_out";
        e.name = "Fade Out";
        e.description = "Fade opacity from 1 to 0 over N frames";
        e.type = EffectDescription::EffectType::Video;
        e.parameters.append({"duration", "Duration (frames)", EffectParameter::Int, 1, 1000, 30, {}});
        effects.append(e);
    }

    // --- Volume (audio) ---
    {
        EffectDescription e;
        e.id = "volume";
        e.name = "Volume";
        e.description = "Audio gain";
        e.type = EffectDescription::EffectType::Audio;
        e.parameters.append({"gain", "Gain", EffectParameter::Double, 0.0, 4.0, 1.0, {}});
        effects.append(e);
    }

    // --- Pan (audio) ---
    {
        EffectDescription e;
        e.id = "pan";
        e.name = "Pan";
        e.description = "Stereo pan (-1=left, 0=center, +1=right)";
        e.type = EffectDescription::EffectType::Audio;
        e.parameters.append({"pan", "Pan", EffectParameter::Double, -1.0, 1.0, 0.0, {}});
        effects.append(e);
    }

    // --- Blur (video) ---
    {
        EffectDescription e;
        e.id = "blur";
        e.name = "Blur";
        e.description = "Gaussian blur (radius in px)";
        e.type = EffectDescription::EffectType::Video;
        e.parameters.append({"radius", "Radius (px)", EffectParameter::Int, 0, 100, 5, {}});
        effects.append(e);
    }

    // --- Brightness/Contrast (video) ---
    {
        EffectDescription e;
        e.id = "brightness";
        e.name = "Brightness";
        e.description = "Brightness & contrast";
        e.type = EffectDescription::EffectType::Video;
        e.parameters.append({"brightness", "Brightness", EffectParameter::Double, -1.0, 1.0, 0.0, {}});
        e.parameters.append({"contrast",   "Contrast",   EffectParameter::Double, -1.0, 1.0, 0.0, {}});
        effects.append(e);
    }

    return effects;
}

} // namespace ve
