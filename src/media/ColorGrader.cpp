#include "media/ColorGrader.h"

#include <QPainter>
#include <QLinearGradient>
#include <cmath>
#include <algorithm>

namespace ve {

namespace {

// Clamp a value to 0..255
inline quint8 clamp8(int v) {
    return static_cast<quint8>(v < 0 ? 0 : (v > 255 ? 255 : v));
}

// Apply temperature shift to RGB.
// Positive temperature = warmer (more red, less blue)
// Negative temperature = cooler (more blue, less red)
inline void applyTemperature(int& r, int& g, int& b, double temp) {
    if (temp == 0.0) return;
    // temp is -100..100; scale to a reasonable delta
    double t = temp / 100.0;
    r = static_cast<int>(r * (1.0 + t * 0.20));
    b = static_cast<int>(b * (1.0 - t * 0.20));
    // g stays the same
}

// Apply tint shift (green <-> magenta)
inline void applyTint(int& r, int& g, int& b, double tint) {
    if (tint == 0.0) return;
    double t = tint / 100.0;
    // Positive tint = more magenta (more R and B, less G)
    r = static_cast<int>(r * (1.0 + t * 0.10));
    b = static_cast<int>(b * (1.0 + t * 0.10));
    g = static_cast<int>(g * (1.0 - t * 0.15));
}

// Apply exposure (multiplicative)
// exposure in -1..1 stops; +1 stop = 2x brighter
inline void applyExposure(int& r, int& g, int& b, double exposure) {
    if (exposure == 0.0) return;
    double factor = std::pow(2.0, exposure);
    r = static_cast<int>(r * factor);
    g = static_cast<int>(g * factor);
    b = static_cast<int>(b * factor);
}

// Apply brightness (additive, simple)
inline void applyBrightness(int& r, int& g, int& b, double brightness) {
    if (brightness == 0.0) return;
    int delta = static_cast<int>(brightness * 100.0);
    r += delta; g += delta; b += delta;
}

// Apply contrast around midpoint (128)
inline void applyContrast(int& r, int& g, int& b, double contrast) {
    if (contrast == 0.0) return;
    double factor = 1.0 + contrast;  // 0..2
    r = static_cast<int>((r - 128) * factor + 128);
    g = static_cast<int>((g - 128) * factor + 128);
    b = static_cast<int>((b - 128) * factor + 128);
}

// Apply saturation (move towards gray)
inline void applySaturation(int& r, int& g, int& b, double saturation) {
    if (saturation == 0.0) return;
    double factor = 1.0 + saturation;
    double gray = 0.299 * r + 0.587 * g + 0.114 * b;
    r = static_cast<int>(gray + (r - gray) * factor);
    g = static_cast<int>(gray + (g - gray) * factor);
    b = static_cast<int>(gray + (b - gray) * factor);
}

// Apply hue rotation (degrees)
inline void applyHue(int& r, int& g, int& b, double hueDeg) {
    if (hueDeg == 0.0) return;
    // Convert RGB -> HSV, rotate H, back to RGB
    double rf = r / 255.0, gf = g / 255.0, bf = b / 255.0;
    double max = std::max({rf, gf, bf});
    double min = std::min({rf, gf, bf});
    double delta = max - min;
    double h = 0.0;
    if (delta > 0.0) {
        if (max == rf)      h = 60.0 * (std::fmod(((gf - bf) / delta), 6.0));
        else if (max == gf) h = 60.0 * (((bf - rf) / delta) + 2.0);
        else                h = 60.0 * (((rf - gf) / delta) + 4.0);
    }
    h = std::fmod(h + hueDeg + 360.0, 360.0);
    double s = (max == 0.0) ? 0.0 : (delta / max);
    double v = max;
    // HSV -> RGB
    double c = v * s;
    double x = c * (1.0 - std::abs(std::fmod(h / 60.0, 2.0) - 1.0));
    double m = v - c;
    double rp, gp, bp;
    if (h < 60)       { rp = c; gp = x; bp = 0; }
    else if (h < 120) { rp = x; gp = c; bp = 0; }
    else if (h < 180) { rp = 0; gp = c; bp = x; }
    else if (h < 240) { rp = 0; gp = x; bp = c; }
    else if (h < 300) { rp = x; gp = 0; bp = c; }
    else              { rp = c; gp = 0; bp = x; }
    r = static_cast<int>((rp + m) * 255.0);
    g = static_cast<int>((gp + m) * 255.0);
    b = static_cast<int>((bp + m) * 255.0);
}

// Apply highlights/shadows (luminance-based masking)
inline void applyHighlightsShadows(int& r, int& g, int& b, double highlights, double shadows) {
    if (highlights == 0.0 && shadows == 0.0) return;
    double lum = (0.299 * r + 0.587 * g + 0.114 * b) / 255.0;
    // Highlights: affects bright areas (lum > 0.5)
    if (highlights != 0.0) {
        double mask = std::max(0.0, (lum - 0.5) * 2.0);  // 0 at lum=0.5, 1 at lum=1.0
        double factor = 1.0 + highlights * mask;
        r = static_cast<int>(r * factor);
        g = static_cast<int>(g * factor);
        b = static_cast<int>(b * factor);
    }
    // Shadows: affects dark areas (lum < 0.5)
    if (shadows != 0.0) {
        double mask = std::max(0.0, (0.5 - lum) * 2.0);  // 0 at lum=0.5, 1 at lum=0.0
        double factor = 1.0 + shadows * mask;
        r = static_cast<int>(r * factor);
        g = static_cast<int>(g * factor);
        b = static_cast<int>(b * factor);
    }
}

// Apply vignette (darken corners)
inline void applyVignette(int& r, int& g, int& b, int x, int y, int w, int h, double vignette) {
    if (vignette == 0.0) return;
    double cx = w / 2.0, cy = h / 2.0;
    double dx = (x - cx) / cx;
    double dy = (y - cy) / cy;
    double dist = std::sqrt(dx*dx + dy*dy);  // 0..~1.41
    double mask = std::max(0.0, std::min(1.0, (dist - 0.5) * 2.0));  // 0 inside, 1 at corners
    double factor = 1.0 - vignette * mask;
    r = static_cast<int>(r * factor);
    g = static_cast<int>(g * factor);
    b = static_cast<int>(b * factor);
}

} // namespace

void ColorGrader::apply(QImage& img, const ColorGrade& g) {
    if (g.isDefault()) return;
    if (img.format() != QImage::Format_RGB32 && img.format() != QImage::Format_ARGB32) {
        img = img.convertToFormat(QImage::Format_ARGB32);
    }

    int w = img.width();
    int h = img.height();
    for (int y = 0; y < h; ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QRgb px = line[x];
            int r = qRed(px);
            int g_ = qGreen(px);
            int b = qBlue(px);

            applyTemperature(r, g_, b, g.temperature);
            applyTint       (r, g_, b, g.tint);
            applyExposure   (r, g_, b, g.exposure);
            applyBrightness (r, g_, b, g.brightness);
            applyContrast   (r, g_, b, g.contrast);
            applyHighlightsShadows(r, g_, b, g.highlights, g.shadows);
            applySaturation (r, g_, b, g.saturation);
            applyHue        (r, g_, b, g.hue);
            applyVignette   (r, g_, b, x, y, w, h, g.vignette);

            line[x] = qRgba(clamp8(r), clamp8(g_), clamp8(b), qAlpha(px));
        }
    }

    // Sharpness/blur would need convolution; skip for now (too slow on CPU)
}

void ColorGrader::histogram(const QImage& img, int binsR[256], int binsG[256], int binsB[256]) {
    std::fill(binsR, binsR + 256, 0);
    std::fill(binsG, binsG + 256, 0);
    std::fill(binsB, binsB + 256, 0);

    QImage sampled = img;
    if (sampled.width() > 256 || sampled.height() > 256) {
        sampled = sampled.scaled(256, 256, Qt::KeepAspectRatio, Qt::FastTransformation);
    }
    if (sampled.format() != QImage::Format_ARGB32 && sampled.format() != QImage::Format_RGB32) {
        sampled = sampled.convertToFormat(QImage::Format_ARGB32);
    }

    int w = sampled.width();
    int h = sampled.height();
    for (int y = 0; y < h; ++y) {
        const QRgb* line = reinterpret_cast<const QRgb*>(sampled.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            QRgb px = line[x];
            binsR[qRed(px)]++;
            binsG[qGreen(px)]++;
            binsB[qBlue(px)]++;
        }
    }
}

QImage ColorGrader::vectorscope(const QImage& img, int size) {
    QImage scope(size, size, QImage::Format_ARGB32);
    scope.fill(Qt::transparent);

    QImage sampled = img;
    if (sampled.width() > 256 || sampled.height() > 256) {
        sampled = sampled.scaled(256, 256, Qt::KeepAspectRatio, Qt::FastTransformation);
    }
    if (sampled.format() != QImage::Format_ARGB32) {
        sampled = sampled.convertToFormat(QImage::Format_ARGB32);
    }

    QPainter p(&scope);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Draw crosshair / circle outline
    p.setPen(QPen(QColor(60, 60, 70), 1));
    p.drawEllipse(0, 0, size - 1, size - 1);
    p.drawLine(size/2, 0, size/2, size);
    p.drawLine(0, size/2, size, size/2);

    // Plot pixels: R-Y vs B-Y
    p.setPen(QPen(QColor(90, 200, 250, 80), 1));
    int w = sampled.width();
    int h = sampled.height();
    int cx = size / 2;
    int cy = size / 2;
    for (int y = 0; y < h; ++y) {
        const QRgb* line = reinterpret_cast<const QRgb*>(sampled.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            QRgb px = line[x];
            int r = qRed(px);
            int g = qGreen(px);
            int b = qBlue(px);
            // Y = 0.299R + 0.587G + 0.114B
            // R-Y = 0.701R - 0.587G - 0.114B
            // B-Y = -0.299R - 0.587G + 0.886B
            double y_ = 0.299 * r + 0.587 * g + 0.114 * b;
            double by = -0.299 * r - 0.587 * g + 0.886 * b;
            double ry = 0.701 * r - 0.587 * g - 0.114 * b;
            // Normalize to scope coords
            int sx = cx + static_cast<int>(by * size / 512.0);
            int sy = cy - static_cast<int>(ry * size / 512.0);
            if (sx >= 0 && sx < size && sy >= 0 && sy < size) {
                p.drawPoint(sx, sy);
            }
            Q_UNUSED(y_);
        }
    }
    return scope;
}

QImage ColorGrader::waveform(const QImage& img, int width) {
    QImage scope(width, 256, QImage::Format_ARGB32);
    scope.fill(Qt::transparent);

    QImage sampled = img.scaledToWidth(width, Qt::FastTransformation);
    if (sampled.format() != QImage::Format_ARGB32) {
        sampled = sampled.convertToFormat(QImage::Format_ARGB32);
    }

    QPainter p(&scope);
    p.setPen(QPen(QColor(90, 200, 250, 120), 1));

    int h = sampled.height();
    for (int x = 0; x < width; ++x) {
        // Sample one column
        for (int y = 0; y < h; y += std::max(1, h / 200)) {
            QRgb px = sampled.pixel(x, y);
            int lum = static_cast<int>(0.299 * qRed(px) + 0.587 * qGreen(px) + 0.114 * qBlue(px));
            int sy = 255 - lum;
            p.drawPoint(x, sy);
        }
    }
    return scope;
}

} // namespace ve
