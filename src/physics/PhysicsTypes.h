#pragma once

#include <cmath>

namespace Physics {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float _x, float _y) : x(_x), y(_y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float lengthSquared() const { return x * x + y * y; }
    float length() const { return std::sqrt(lengthSquared()); }

    Vec2 normalized() const {
        float l = length();
        if (l > 0.00001f) return *this / l;
        return {0.0f, 0.0f};
    }

    static float dot(const Vec2& a, const Vec2& b) {
        return a.x * b.x + a.y * b.y;
    }

    static float cross(const Vec2& a, const Vec2& b) {
        return a.x * b.y - a.y * b.x;
    }
};

struct ContactPoint {
    bool hasContact = false;
    Vec2 point;
    Vec2 normal;
    float penetration = 0.0f;
};

} // namespace Physics
