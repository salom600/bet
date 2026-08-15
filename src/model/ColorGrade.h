#pragma once

namespace ve {

/// Color grading parameters applied during frame rendering.
/// All values are in "natural" units (e.g. temperature in -100..100,
/// exposure in stops, saturation in percent).
struct ColorGrade {
    // White balance
    double temperature = 0.0;   // -100 (cool) .. +100 (warm)
    double tint        = 0.0;   // -100 (green) .. +100 (magenta)

    // Tone
    double exposure    = 0.0;   // -1.0 .. +1.0 stops
    double contrast    = 0.0;   // -1.0 .. +1.0
    double brightness  = 0.0;   // -1.0 .. +1.0
    double highlights  = 0.0;   // -1.0 .. +1.0
    double shadows     = 0.0;   // -1.0 .. +1.0

    // Color
    double saturation  = 0.0;   // -1.0 .. +1.0
    double vibrance    = 0.0;   // -1.0 .. +1.0
    double hue         = 0.0;   // -180 .. +180 degrees

    // Detail
    double sharpness   = 0.0;   // 0.0 .. 1.0
    double blur        = 0.0;   // 0.0 .. 1.0

    // Vignette
    double vignette    = 0.0;   // 0.0 .. 1.0

    bool isDefault() const {
        return temperature == 0.0 && tint == 0.0 && exposure == 0.0 &&
               contrast == 0.0 && brightness == 0.0 && highlights == 0.0 &&
               shadows == 0.0 && saturation == 0.0 && vibrance == 0.0 &&
               hue == 0.0 && sharpness == 0.0 && blur == 0.0 && vignette == 0.0;
    }

    void reset() {
        *this = ColorGrade{};
    }
};

} // namespace ve
