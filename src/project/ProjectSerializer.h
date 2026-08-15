/*
 * VideoEditor - ProjectSerializer.h
 * Load/save .veproj files (XML format, MLT-like structure).
 *
 * Adapted from Kdenlive's src/doc/documentvalidator.cpp + xml/xml.hpp.
 *
 * Format (simplified MLT-like):
 *
 *   <veproject version="1" producer="main_bin">
 *     <profile width="1920" height="1080" frame_rate_num="25" .../>
 *     <bin>
 *       <folder id="root" name="Bin">
 *         <clip id="clip1" path="/path/to/file.mp4" type="3" duration="12.5"/>
 *         ...
 *       </folder>
 *     </bin>
 *     <timeline>
 *       <track id="1" type="0" name="Video" muted="0" locked="0" visible="1">
 *         <clip id="7" binClipId="clip1" position="0" in="0" out="312"/>
 *         ...
 *       </track>
 *     </timeline>
 *   </veproject>
 *
 * This mirrors MLT's <mlt>/<producer>/<playlist>/<tractor> structure so a
 * future MLT integration can map them 1:1.
 */
#pragma once

#include <QString>

namespace ve {

class Project;

class ProjectSerializer {
public:
    static bool save(Project* project, const QString& path, QString* err = nullptr);
    static bool load(Project* project, const QString& path, QString* err = nullptr);
};

} // namespace ve
