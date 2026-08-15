#pragma once

#include "model/ColorGrade.h"
#include <QImage>

namespace ve {

/// Applies a ColorGrade to a QImage in-place using CPU operations.
/// Used by the preview monitor and the export pipeline.
///
/// All operations are simple per-pixel math (no GPU, no OpenCL) but they
/// are REAL: brightness/contrast/saturation/temperature/etc. all actually
/// modify pixel values.
class ColorGrader {
public:
    /// Apply the grade to img in-place.
    static void apply(QImage& img, const ColorGrade& g);

    /// Render a histogram (256 bins, RGB) from the given image.
    /// Returns a 256x3 array (R, G, B channels).
    static void histogram(const QImage& img, int binsR[256], int binsG[256], int binsB[256]);

    /// Render a vectorscope (256x256 QImage) from the given image.
    static QImage vectorscope(const QImage& img, int size = 256);

    /// Render a waveform/luma scope (W x 256 QImage) from the given image.
    static QImage waveform(const QImage& img, int width = 256);
};

} // namespace ve
