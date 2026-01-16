#pragma once

#include "widget.hpp"
#include "layouts.hpp"
#include <sstream>
#include <iomanip>

namespace MetaUI {

// Forward declaration
class Renderer;

// ============================================================================
// Text Widget
// ============================================================================

class Text : public Widget {
public:
    explicit Text(const std::string& text = "") : text_(text) {}
    
    Text& text(const std::string& t) { text_ = t; return *this; }
    Text& font(const std::string& family, float size = 14.0f) {
        textStyle_.fontFamily = family;
        textStyle_.fontSize = size;
        return *this;
    }
    Text& fontSize(float size) { textStyle_.fontSize = size; return *this; }
    Text& color(const Color& c) { textStyle_.color = c; return *this; }
    Text& bold(bool b = true) { textStyle_.bold = b; return *this; }
    Text& italic(bool i = true) { textStyle_.italic = i; return *this; }
    Text& align(TextStyle::Align a) { textStyle_.align = a; return *this; }
    Text& valign(TextStyle::VAlign a) { textStyle_.valign = a; return *this; }
    
    const std::string& getText() const { return text_; }
    const TextStyle& textStyle() const { return textStyle_; }
    
    Size measureContent(Size available) override;
    void render(Renderer& renderer) override;
    
private:
    std::string text_;
    TextStyle textStyle_;
};

// ============================================================================
// Image Widget
// ============================================================================

class Image : public Widget {
public:
    explicit Image(const std::string& path = "") : imagePath_(path) {}
    
    Image& path(const std::string& p) { imagePath_ = p; return *this; }
    Image& fit(bool f) { fit_ = f; return *this; }
    
    const std::string& imagePath() const { return imagePath_; }
    bool shouldFit() const { return fit_; }
    
    void render(Renderer& renderer) override;
    
private:
    std::string imagePath_;
    bool fit_ = true;  // Fit to bounds or original size
};

// ============================================================================
// Button Widget
// ============================================================================

class Button : public Widget {
public:
    explicit Button(const std::string& label = "") : label_(label) {
        style_.padding = Padding(10, 20, 10, 20);
        style_.borderRadius = BorderRadius(4);
        style_.background = Color::fromHex(0x3b82f6ff);
        
        textStyle_.color = Color(1, 1, 1, 1);
        textStyle_.fontSize = 14;
        textStyle_.align = TextStyle::Align::Center;
    }
    
    Button& label(const std::string& l) { label_ = l; return *this; }
    Button& textColor(const Color& c) { textStyle_.color = c; return *this; }
    
    // State-specific styling
    Button& hoverStyle(const Color& bg) { hoverBg_ = bg; return *this; }
    Button& activeStyle(const Color& bg) { activeBg_ = bg; return *this; }
    
    const std::string& getLabel() const { return label_; }
    
    Size measureContent(Size available) override;
    void render(Renderer& renderer) override;
    
    bool handleMouseMove(const MouseEvent& event) override {
        bool result = Widget::handleMouseMove(event);
        if (hovered_ != wasHovered_) {
            wasHovered_ = hovered_;
        }
        return result;
    }
    
    bool handleMouseButton(const MouseEvent& event) override {
        pressed_ = event.pressed && bounds_.contains(event.position);
        return Widget::handleMouseButton(event);
    }
    
private:
    std::string label_;
    TextStyle textStyle_;
    Color hoverBg_ = Color::fromHex(0x2563ebff);
    Color activeBg_ = Color::fromHex(0x1d4ed8ff);
    bool wasHovered_ = false;
    bool pressed_ = false;
};

// ============================================================================
// TextInput Widget
// ============================================================================

class TextInput : public Widget {
public:
    explicit TextInput(const std::string& placeholder = "") 
        : placeholder_(placeholder) {
        style_.padding = Padding(8, 12, 8, 12);
        style_.background = Color(0.1f, 0.1f, 0.1f, 1.0f);
        style_.border(Color(0.3f, 0.3f, 0.3f, 1.0f), 1);
        style_.borderRadius = BorderRadius(4);
        
        textStyle_.fontSize = 14;
        textStyle_.color = Color(1, 1, 1, 1);
    }
    
    TextInput& placeholder(const std::string& p) { placeholder_ = p; return *this; }
    TextInput& value(const std::string& v) { text_ = v; cursorPos_ = v.length(); return *this; }
    TextInput& onChange(std::function<void(const std::string&)> handler) {
        onChangeHandler_ = std::move(handler);
        return *this;
    }
    
    const std::string& value() const { return text_; }
    
    Size measureContent(Size available) override {
        return Size(200, textStyle_.fontSize * 1.5f);
    }
    
    void render(Renderer& renderer) override;
    
    bool handleKeyEvent(const KeyEvent& event) override {
        if (!focused_) return false;
        
        if (event.pressed) {
            if (event.keysym == 0xff08) {  // Backspace
                if (cursorPos_ > 0) {
                    text_.erase(cursorPos_ - 1, 1);
                    cursorPos_--;
                    if (onChangeHandler_) onChangeHandler_(text_);
                }
            } else if (event.keysym == 0xffff) {  // Delete
                if (cursorPos_ < text_.length()) {
                    text_.erase(cursorPos_, 1);
                    if (onChangeHandler_) onChangeHandler_(text_);
                }
            } else if (event.keysym == 0xff51) {  // Left arrow
                if (cursorPos_ > 0) cursorPos_--;
            } else if (event.keysym == 0xff53) {  // Right arrow
                if (cursorPos_ < text_.length()) cursorPos_++;
            } else if (!event.text.empty()) {
                text_.insert(cursorPos_, event.text);
                cursorPos_ += event.text.length();
                if (onChangeHandler_) onChangeHandler_(text_);
            }
        }
        return true;
    }
    
    bool handleMouseButton(const MouseEvent& event) override {
        bool clicked = Widget::handleMouseButton(event);
        if (clicked && event.pressed) {
            setFocus(true);
        }
        return clicked;
    }
    
private:
    std::string text_;
    std::string placeholder_;
    TextStyle textStyle_;
    size_t cursorPos_ = 0;
    std::function<void(const std::string&)> onChangeHandler_;
};

// ============================================================================
// Slider Widget
// ============================================================================

class Slider : public Widget {
public:
    Slider(float min = 0.0f, float max = 1.0f, float value = 0.5f)
        : min_(min), max_(max), value_(value) {
        style_.background = Color(0.2f, 0.2f, 0.2f, 1.0f);
        style_.borderRadius = BorderRadius(3);
    }
    
    Slider& range(float min, float max) { min_ = min; max_ = max; return *this; }
    Slider& value(float v) { value_ = std::clamp(v, min_, max_); return *this; }
    Slider& onChange(std::function<void(float)> handler) {
        onChangeHandler_ = std::move(handler);
        return *this;
    }
    
    Slider& trackColor(const Color& c) { style_.background = c; return *this; }
    Slider& thumbColor(const Color& c) { thumbColor_ = c; return *this; }
    Slider& fillColor(const Color& c) { fillColor_ = c; return *this; }
    
    float getValue() const { return value_; }
    
    Size measureContent(Size available) override {
        return Size(200, 20);
    }
    
    void render(Renderer& renderer) override;
    
    bool handleMouseButton(const MouseEvent& event) override {
        if (event.button == MouseButton::Left) {
            if (event.pressed && bounds_.contains(event.position)) {
                dragging_ = true;
                updateValue(event.position.x);
                return true;
            } else if (!event.pressed) {
                dragging_ = false;
            }
        }
        return false;
    }
    
    bool handleMouseMove(const MouseEvent& event) override {
        Widget::handleMouseMove(event);
        
        if (dragging_) {
            updateValue(event.position.x);
            return true;
        }
        return false;
    }
    
private:
    float min_, max_, value_;
    Color thumbColor_ = Color::fromHex(0x3b82f6ff);
    Color fillColor_ = Color::fromHex(0x60a5faff);
    bool dragging_ = false;
    std::function<void(float)> onChangeHandler_;
    
    void updateValue(float mouseX) {
        float normalized = (mouseX - bounds_.x) / bounds_.width;
        value_ = std::clamp(min_ + normalized * (max_ - min_), min_, max_);
        if (onChangeHandler_) {
            onChangeHandler_(value_);
        }
    }
};

// ============================================================================
// Checkbox Widget
// ============================================================================

class Checkbox : public Widget {
public:
    explicit Checkbox(bool checked = false) : checked_(checked) {
        style_.background = Color(0.2f, 0.2f, 0.2f, 1.0f);
        style_.border(Color(0.4f, 0.4f, 0.4f, 1.0f), 1);
        style_.borderRadius = BorderRadius(3);
    }
    
    Checkbox& checked(bool c) { checked_ = c; return *this; }
    Checkbox& onToggle(std::function<void(bool)> handler) {
        onToggleHandler_ = std::move(handler);
        return *this;
    }
    
    bool isChecked() const { return checked_; }
    
    Size measureContent(Size available) override {
        return Size(20, 20);
    }
    
    void render(Renderer& renderer) override;
    
    bool handleMouseButton(const MouseEvent& event) override {
        if (event.pressed && event.button == MouseButton::Left) {
            if (bounds_.contains(event.position)) {
                checked_ = !checked_;
                if (onToggleHandler_) {
                    onToggleHandler_(checked_);
                }
                return true;
            }
        }
        return false;
    }
    
private:
    bool checked_;
    std::function<void(bool)> onToggleHandler_;
};

// ============================================================================
// Progress Bar Widget
// ============================================================================

class ProgressBar : public Widget {
public:
    explicit ProgressBar(float progress = 0.0f) : progress_(progress) {
        style_.background = Color(0.2f, 0.2f, 0.2f, 1.0f);
        style_.borderRadius = BorderRadius(3);
    }
    
    ProgressBar& progress(float p) { progress_ = std::clamp(p, 0.0f, 1.0f); return *this; }
    ProgressBar& fillColor(const Color& c) { fillColor_ = c; return *this; }
    
    float getProgress() const { return progress_; }
    
    Size measureContent(Size available) override {
        return Size(200, 10);
    }
    
    void render(Renderer& renderer) override;
    
private:
    float progress_;
    Color fillColor_ = Color::fromHex(0x3b82f6ff);
};

// ============================================================================
// Spacer Widget (for layouts)
// ============================================================================

class Spacer : public Widget {
public:
    explicit Spacer(float size = 10) {
        widthSpec_ = SizeSpec::fixed(size);
        heightSpec_ = SizeSpec::fixed(size);
    }
    
    static WidgetPtr flexible() {
        auto spacer = std::make_shared<Spacer>();
        spacer->widthSpec_ = SizeSpec::fill();
        spacer->heightSpec_ = SizeSpec::fill();
        return spacer;
    }
    
    void render(Renderer&) override {}  // Invisible
};

// ============================================================================
// Divider Widget
// ============================================================================

class Divider : public Widget {
public:
    explicit Divider(Direction dir = Direction::Horizontal) : direction_(dir) {
        if (dir == Direction::Horizontal) {
            heightSpec_ = SizeSpec::fixed(1);
            widthSpec_ = SizeSpec::fill();
        } else {
            widthSpec_ = SizeSpec::fixed(1);
            heightSpec_ = SizeSpec::fill();
        }
        style_.background = Color(0.3f, 0.3f, 0.3f, 1.0f);
    }
    
private:
    Direction direction_;
};

} // namespace MetaUI
