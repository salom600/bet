/*
 * VideoEditor - Profile.h
 * Project profile: resolution, fps, aspect ratios.
 * Adapted from Kdenlive's src/profiles/profilemodel.hpp.
 *
 * The profile describes the OUTPUT format of the project (e.g. 1920x1080@25fps
 * 16:9 SAR 1:1 progressive BT.709). All clips are scaled to fit at render
 * time, so the project can mix footage from different cameras.
 */
#pragma once

#include <QString>
#include <QSize>

namespace ve {

struct Profile {
    int     width         = 1920;
    int     height        = 1080;
    int     fps_num       = 25;
    int     fps_den       = 1;
    int     sar_num       = 1;   // sample aspect ratio
    int     sar_den       = 1;
    int     dar_num       = 16;  // display aspect ratio (auto from w/h if 0)
    int     dar_den       = 9;
    int     colorspace    = 709; // 709=BT.709 HD, 601=BT.601 SD
    bool    progressive   = true;
    QString description   = "HD 1080p 25 fps";

    double fps() const { return fps_den ? double(fps_num) / fps_den : 25.0; }
    double sar() const { return sar_den ? double(sar_num) / sar_den : 1.0; }
    double dar() const {
        if (dar_num && dar_den) return double(dar_num) / dar_den;
        return double(width) / height;
    }
    QSize  size()  const { return QSize(width, height); }
    bool   isValid() const { return width > 0 && height > 0 && fps_num > 0; }

    // Round width to nearest multiple of 8 (MLT requirement).
    void adjustDimensions() {
        width = (width / 8) * 8;
        if (width < 8) width = 8;
    }

    static Profile fromDescription(const QString& desc);
};

} // namespace ve
