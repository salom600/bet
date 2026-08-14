/*
 * VideoEditor - GenTime.h
 * Frame-aware time representation.
 * Adapted from Kdenlive's src/utils/gentime.h.
 *
 * GenTime stores time as a double-precision seconds value, but exposes
 * frame-quantized operations so that comparisons are stable across fps.
 */
#pragma once

#include <QString>
#include <cmath>

namespace ve {

class GenTime {
public:
    GenTime() : m_time(0.0) {}
    explicit GenTime(double seconds) : m_time(seconds) {}
    GenTime(int frames, double fps) : m_time(fps > 0 ? frames / fps : 0.0) {}

    double seconds() const { return m_time; }
    double ms()      const { return m_time * 1000.0; }
    int    frames(double fps) const { return static_cast<int>(std::round(m_time * fps)); }

    // Formatted as HH:MM:SS.FF (FF = frame number at given fps)
    QString toTimecode(double fps) const {
        int total = frames(fps);
        int f  = total % static_cast<int>(std::round(fps));
        int s  = (total / static_cast<int>(std::round(fps))) % 60;
        int m  = (total / static_cast<int>(std::round(fps)) / 60) % 60;
        int h  = total / static_cast<int>(std::round(fps)) / 3600;
        return QString("%1:%2:%3.%4")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'))
            .arg(f, 2, 10, QChar('0'));
    }

    // Operators
    GenTime operator-() const { return GenTime(-m_time); }
    GenTime& operator+=(GenTime o) { m_time += o.m_time; return *this; }
    GenTime& operator-=(GenTime o) { m_time -= o.m_time; return *this; }
    GenTime operator+(GenTime o) const { return GenTime(m_time + o.m_time); }
    GenTime operator-(GenTime o) const { return GenTime(m_time - o.m_time); }
    GenTime operator*(double s) const { return GenTime(m_time * s); }
    GenTime operator/(double s) const { return GenTime(s != 0.0 ? m_time / s : 0.0); }

    // Equality considers times within 1/4 frame at the static fps to be equal.
    bool operator<(GenTime o)  const { return m_time <  o.m_time - s_delta; }
    bool operator>(GenTime o)  const { return m_time >  o.m_time + s_delta; }
    bool operator>=(GenTime o) const { return !(*this < o); }
    bool operator<=(GenTime o) const { return !(*this > o); }
    bool operator==(GenTime o) const { return !(*this < o) && !(*this > o); }
    bool operator!=(GenTime o) const { return !(*this == o); }

    static void setFps(double fps) {
        s_fps  = fps;
        s_delta = (fps > 0) ? (0.5 / fps) : 0.0001;
    }
    static double fps() { return s_fps; }

private:
    double m_time;
    static double s_fps;
    static double s_delta;
};

} // namespace ve
