#pragma once
#include <algorithm>
#include <cmath>

namespace jah
{
constexpr double kPi = 3.14159265358979323846;

inline double dbToLin (double db) { return std::pow (10.0, db / 20.0); }
inline double clampd (double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }

struct OnePole
{
    double G = 0.1, s = 0.0;
    void set (double fc, double fs)
    {
        const double g = std::tan (kPi * clampd (fc, 1.0, fs * 0.45) / fs);
        G = g / (1.0 + g);
    }
    double lp (double x) { const double v = (x - s) * G; const double y = v + s; s = y + v; return y; }
    double hp (double x) { return x - lp (x); }
    void reset() { s = 0.0; }
};

} // namespace jah
