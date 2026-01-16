#pragma once

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <cmath>

namespace MetaUI {

// ============================================================================
// Core Types & Utilities
// ============================================================================

struct Color {
    float r, g, b, a;
    
    Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}
    
    static Color fromHex(uint32_t hex) {
        return Color(
            ((hex >> 24) & 0xFF) / 255.0f,
            ((hex >> 16) & 0xFF) / 255.0f,
            ((hex >> 8) & 0xFF) / 255.0f,
            (hex & 0xFF) / 255.0f
        );
    }
    
    static Color fromRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    }
    
    Color withAlpha(float alpha) const {
        return Color(r, g, b, alpha);
    }
};

struct Point {
    float x, y;
    Point(float x = 0, float y = 0) : x(x), y(y) {}
    
    Point operator+(const Point& other) const {
        return Point(x + other.x, y + other.y);
    }
    
    Point operator-(const Point& other) const {
        return Point(x - other.x, y - other.y);
    }
};

struct Size {
    float width, height;
    Size(float w = 0, float h = 0) : width(w), height(h) {}
    
    bool contains(float w, float h) const {
        return w >= 0 && w <= width && h >= 0 && h <= height;
    }
};

struct Rect {
    float x, y, width, height;
    
    Rect(float x = 0, float y = 0, float w = 0, float h = 0)
        : x(x), y(y), width(w), height(h) {}
    
    bool contains(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }
    
    bool contains(const Point& p) const {
        return contains(p.x, p.y);
    }
    
    Point topLeft() const { return Point(x, y); }
    Point topRight() const { return Point(x + width, y); }
    Point bottomLeft() const { return Point(x, y + height); }
    Point bottomRight() const { return Point(x + width, y + height); }
    Point center() const { return Point(x + width/2, y + height/2); }
};

struct Padding {
    float top, right, bottom, left;
    
    Padding(float all = 0) : top(all), right(all), bottom(all), left(all) {}
    Padding(float vertical, float horizontal) 
        : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {}
    Padding(float t, float r, float b, float l)
        : top(t), right(r), bottom(b), left(l) {}
    
    float horizontal() const { return left + right; }
    float vertical() const { return top + bottom; }
};

struct BorderRadius {
    float topLeft, topRight, bottomRight, bottomLeft;
    
    BorderRadius(float all = 0) 
        : topLeft(all), topRight(all), bottomRight(all), bottomLeft(all) {}
    BorderRadius(float tl, float tr, float br, float bl)
        : topLeft(tl), topRight(tr), bottomRight(br), bottomLeft(bl) {}
};

// ============================================================================
// Layout System
// ============================================================================

enum class SizeConstraint {
    Fixed,      // Fixed size
    Fill,       // Fill available space
    Content,    // Size based on content
    Percent,    // Percentage of parent
};

struct SizeSpec {
    SizeConstraint constraint = SizeConstraint::Content;
    float value = 0;  // Used for Fixed and Percent
    
    static SizeSpec fixed(float v) { 
        SizeSpec s; s.constraint = SizeConstraint::Fixed; s.value = v; return s;
    }
    static SizeSpec fill() { 
        SizeSpec s; s.constraint = SizeConstraint::Fill; return s;
    }
    static SizeSpec content() { 
        SizeSpec s; s.constraint = SizeConstraint::Content; return s;
    }
    static SizeSpec percent(float v) { 
        SizeSpec s; s.constraint = SizeConstraint::Percent; s.value = v; return s;
    }
};

enum class Alignment {
    Start,
    Center,
    End,
    Stretch
};

enum class Direction {
    Horizontal,
    Vertical
};

// ============================================================================
// Event System
// ============================================================================

enum class MouseButton {
    Left,
    Right,
    Middle,
    Button4,
    Button5
};

enum class KeyMod {
    None = 0,
    Shift = 1 << 0,
    Ctrl = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3
};

struct MouseEvent {
    Point position;
    Point delta;  // For motion
    MouseButton button;
    bool pressed;
    uint32_t mods;
};

struct KeyEvent {
    uint32_t keycode;
    uint32_t keysym;
    bool pressed;
    uint32_t mods;
    std::string text;  // For text input
};

struct ScrollEvent {
    Point position;
    float deltaX;
    float deltaY;
};

// ============================================================================
// Style System
// ============================================================================

struct TextStyle {
    std::string fontFamily = "sans-serif";
    float fontSize = 14.0f;
    Color color = Color(1, 1, 1, 1);
    bool bold = false;
    bool italic = false;
    float lineHeight = 1.4f;
    
    enum class Align { Left, Center, Right } align = Align::Left;
    enum class VAlign { Top, Middle, Bottom } valign = VAlign::Middle;
};

struct BoxStyle {
    Color background = Color(0, 0, 0, 0);
    Color borderColor = Color(0, 0, 0, 0);
    float borderWidth = 0.0f;
    BorderRadius borderRadius;
    Padding padding;
    Padding margin;
    
    // Shadow
    bool hasShadow = false;
    Color shadowColor = Color(0, 0, 0, 0.3f);
    Point shadowOffset = Point(0, 2);
    float shadowBlur = 4.0f;
    
    // Gradient
    bool hasGradient = false;
    Color gradientStart;
    Color gradientEnd;
    float gradientAngle = 0.0f;  // in degrees
};

// ============================================================================
// Animation System
// ============================================================================

enum class EasingCurve {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    Bounce,
    Elastic
};

float easeValue(float t, EasingCurve curve) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    
    switch (curve) {
        case EasingCurve::Linear:
            return t;
        
        case EasingCurve::EaseIn:
            return t * t;
        
        case EasingCurve::EaseOut:
            return 1.0f - (1.0f - t) * (1.0f - t);
        
        case EasingCurve::EaseInOut:
            if (t < 0.5f) return 2.0f * t * t;
            return 1.0f - std::pow(-2.0f * t + 2.0f, 2) / 2.0f;
        
        case EasingCurve::Bounce: {
            float n1 = 7.5625f;
            float d1 = 2.75f;
            if (t < 1.0f / d1) {
                return n1 * t * t;
            } else if (t < 2.0f / d1) {
                t -= 1.5f / d1;
                return n1 * t * t + 0.75f;
            } else if (t < 2.5f / d1) {
                t -= 2.25f / d1;
                return n1 * t * t + 0.9375f;
            } else {
                t -= 2.625f / d1;
                return n1 * t * t + 0.984375f;
            }
        }
        
        case EasingCurve::Elastic: {
            float c4 = (2.0f * 3.14159f) / 3.0f;
            return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
        }
    }
    return t;
}

template<typename T>
class Animation {
public:
    Animation(T start, T end, float duration, EasingCurve curve = EasingCurve::EaseOut)
        : start_(start), end_(end), duration_(duration), curve_(curve)
        , elapsed_(0), running_(false) {}
    
    void start() { running_ = true; elapsed_ = 0; }
    void stop() { running_ = false; }
    void reset() { elapsed_ = 0; }
    
    bool update(float dt) {
        if (!running_) return false;
        
        elapsed_ += dt;
        if (elapsed_ >= duration_) {
            elapsed_ = duration_;
            running_ = false;
            return true;  // Animation complete
        }
        return false;
    }
    
    T value() const {
        float t = elapsed_ / duration_;
        float eased = easeValue(t, curve_);
        return lerp(start_, end_, eased);
    }
    
    bool isRunning() const { return running_; }
    
private:
    T start_, end_;
    float duration_;
    EasingCurve curve_;
    float elapsed_;
    bool running_;
    
    template<typename U>
    U lerp(const U& a, const U& b, float t) const {
        return a + (b - a) * t;
    }
};

// Specialization for Color
inline Color Animation<Color>::lerp(const Color& a, const Color& b, float t) const {
    return Color(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    );
}

} // namespace MetaUI
