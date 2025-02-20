#pragma once
inline constexpr double lerp(double a, double b, double t) {
    return a + t * (b - a);
}