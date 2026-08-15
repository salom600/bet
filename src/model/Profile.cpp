#include "model/Profile.h"

namespace ve {

Profile Profile::fromDescription(const QString& desc) {
    Profile p;
    if (desc.startsWith("1080p", Qt::CaseInsensitive)) {
        p.width = 1920; p.height = 1080;
        p.fps_num = desc.contains("30") ? 30 : (desc.contains("50") ? 50 : 25);
        p.fps_den = 1;
        p.dar_num = 16; p.dar_den = 9;
        p.progressive = true;
        p.description = QString("HD 1080p %1 fps").arg(p.fps_num);
    } else if (desc.startsWith("720p", Qt::CaseInsensitive)) {
        p.width = 1280; p.height = 720;
        p.fps_num = desc.contains("30") ? 30 : (desc.contains("50") ? 50 : 25);
        p.fps_den = 1;
        p.dar_num = 16; p.dar_den = 9;
        p.progressive = true;
        p.description = QString("HD 720p %1 fps").arg(p.fps_num);
    } else if (desc.startsWith("4k", Qt::CaseInsensitive) || desc.startsWith("2160p", Qt::CaseInsensitive)) {
        p.width = 3840; p.height = 2160;
        p.fps_num = desc.contains("60") ? 60 : 25;
        p.fps_den = 1;
        p.dar_num = 16; p.dar_den = 9;
        p.progressive = true;
        p.description = QString("UHD 4K %1 fps").arg(p.fps_num);
    } else if (desc.startsWith("square", Qt::CaseInsensitive)) {
        p.width = 1080; p.height = 1080;
        p.fps_num = 30; p.fps_den = 1;
        p.dar_num = 1; p.dar_den = 1;
        p.progressive = true;
        p.description = "Square 1080 30 fps";
    } else if (desc.startsWith("vertical", Qt::CaseInsensitive)) {
        p.width = 1080; p.height = 1920;
        p.fps_num = 30; p.fps_den = 1;
        p.dar_num = 9; p.dar_den = 16;
        p.progressive = true;
        p.description = "Vertical 1080x1920 30 fps";
    }
    return p;
}

} // namespace ve
