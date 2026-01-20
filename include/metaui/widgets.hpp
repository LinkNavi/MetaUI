#pragma once

#include "widget.hpp"
#include "layouts.hpp"
#include "renderer.hpp"
#include <sstream>
#include <iomanip>

namespace MetaUI {

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
    Text& lineHeight(float h) { textStyle_.lineHeight = h; return *this; }
    Text& wrap(bool w) { wrap_ = w; return *this; }
    Text& maxWidth(float w) { maxWidth_ = w; return *this; }
    
    const std::string& getText() const { return text_; }
    const TextStyle& textStyle() const { return textStyle_; }
    
    Size measureContent(Size available) override {
        if (text_.empty()) return Size(0, textStyle_.fontSize);
        return Size(text_.length() * textStyle_.fontSize * 0.6f, 
                    textStyle_.fontSize * textStyle_.lineHeight);
    }
    
    void render(Renderer& renderer) override {
        Widget::render(renderer);
        
        if (text_.empty()) return;
        
        Font* font = renderer.loadFont(textStyle_.fontFamily, textStyle_.fontSize);
        if (!font) return;
        
        Size textSize = font->measureText(text_);
        
        // Calculate position based on alignment
        Point textPos = contentBounds_.topLeft();
        
        switch (textStyle_.align) {
            case TextStyle::Align::Center:
                textPos.x += (contentBounds_.width - textSize.width) / 2;
                break;
            case TextStyle::Align::Right:
                textPos.x += contentBounds_.width - textSize.width;
                break;
            default:
                break;
        }
        
        switch (textStyle_.valign) {
            case TextStyle::VAlign::Middle:
                textPos.y += (contentBounds_.height - textSize.height) / 2;
                break;
            case TextStyle::VAlign::Bottom:
                textPos.y += contentBounds_.height - textSize.height;
                break;
            default:
                break;
        }
        
        renderer.drawText(text_, textPos, font, textStyle_.color, textStyle_);
    }
    
private:
    std::string text_;
    TextStyle textStyle_;
    bool wrap_ = false;
    float maxWidth_ = 0;
};

// ============================================================================
// Image Widget
// ============================================================================

class Image : public Widget {
public:
    explicit Image(const std::string& path = "") : imagePath_(path) {}
    
    Image& path(const std::string& p) { 
        imagePath_ = p; 
        texture_ = nullptr;  // Force reload
        return *this; 
    }
    Image& fit(bool f) { fit_ = f; return *this; }
    Image& preserveAspect(bool p) { preserveAspect_ = p; return *this; }
    Image& opacity(float o) { opacity_ = std::clamp(o, 0.0f, 1.0f); return *this; }
    Image& tint(const Color& c) { tint_ = c; hasTint_ = true; return *this; }
    
    const std::string& imagePath() const { return imagePath_; }
    bool shouldFit() const { return fit_; }
    
    Size measureContent(Size available) override {
        // If we have fixed size, use that
        if (widthSpec_.constraint == SizeConstraint::Fixed && 
            heightSpec_.constraint == SizeConstraint::Fixed) {
            return Size(widthSpec_.value, heightSpec_.value);
        }
        
        // Otherwise use image size or default
        if (texture_ && texture_->valid()) {
            return Size(texture_->width(), texture_->height());
        }
        return Size(100, 100);  // Default placeholder size
    }
    
    void render(Renderer& renderer) override {
        Widget::render(renderer);
        
        if (imagePath_.empty()) {
            // Draw placeholder
            renderer.drawRect(contentBounds_, Color(0.3f, 0.3f, 0.3f, 0.5f));
            return;
        }
        
        // Load texture if needed
        if (!texture_) {
            texture_ = renderer.loadImage(imagePath_);
        }
        
        if (texture_ && texture_->valid()) {
            if (preserveAspect_) {
                renderer.drawImageScaled(*texture_, contentBounds_, true, opacity_);
            } else {
                renderer.drawImage(*texture_, contentBounds_, opacity_);
            }
        } else {
            // Draw error placeholder
            renderer.drawRect(contentBounds_, Color(0.5f, 0.2f, 0.2f, 0.5f));
        }
    }
    
private:
    std::string imagePath_;
    Texture* texture_ = nullptr;
    bool fit_ = true;
    bool preserveAspect_ = true;
    float opacity_ = 1.0f;
    Color tint_;
    bool hasTint_ = false;
};

// ============================================================================
// Icon Widget (for icon fonts or small images)
// ============================================================================

class Icon : public Widget {
public:
    explicit Icon(const std::string& name = "", float size = 24.0f) 
        : name_(name), size_(size) {
        widthSpec_ = SizeSpec::fixed(size);
        heightSpec_ = SizeSpec::fixed(size);
    }
    
    Icon& name(const std::string& n) { name_ = n; return *this; }
    Icon& size(float s) { 
        size_ = s; 
        widthSpec_ = SizeSpec::fixed(s);
        heightSpec_ = SizeSpec::fixed(s);
        return *this; 
    }
    Icon& color(const Color& c) { color_ = c; return *this; }
    
    void render(Renderer& renderer) override {
        Widget::render(renderer);
        
        // For now, draw a simple shape as placeholder
        // In production, would load from icon font or sprite sheet
        float cx = contentBounds_.x + contentBounds_.width / 2;
        float cy = contentBounds_.y + contentBounds_.height / 2;
        float r = std::min(contentBounds_.width, contentBounds_.height) / 2 - 2;
        
        renderer.drawRoundedRect(
            Rect(cx - r, cy - r, r * 2, r * 2),
            BorderRadius(r),
            color_
        );
    }
    
private:
    std::string name_;
    float size_;
    Color color_ = Color(1, 1, 1, 1);
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
    Button& fontSize(float s) { textStyle_.fontSize = s; return *this; }
    Button& hoverStyle(const Color& bg) { hoverBg_ = bg; return *this; }
    Button& activeStyle(const Color& bg) { activeBg_ = bg; return *this; }
    Button& icon(const std::string& iconPath) { iconPath_ = iconPath; return *this; }
    
    const std::string& getLabel() const { return label_; }
    
    Size measureContent(Size available) override {
        return Size(label_.length() * textStyle_.fontSize * 0.6f + 40, 
                    textStyle_.fontSize * 1.4f + 20);
    }
    
    void render(Renderer& renderer) override {
        Color bg = style_.background;
        if (pressed_) {
            bg = activeBg_;
        } else if (hovered_) {
            bg = hoverBg_;
        }
        
        BoxStyle tempStyle = style_;
        tempStyle.background = bg;
        style_ = tempStyle;
        
        Widget::render(renderer);
        
        Font* font = renderer.loadFont(textStyle_.fontFamily, textStyle_.fontSize);
        if (font) {
            Size textSize = font->measureText(label_);
            Point textPos(
                contentBounds_.x + (contentBounds_.width - textSize.width) / 2,
                contentBounds_.y + (contentBounds_.height - textSize.height) / 2
            );
            renderer.drawText(label_, textPos, font, textStyle_.color, textStyle_);
        }
    }
    
    bool handleMouseMove(const MouseEvent& event) override {
        bool result = Widget::handleMouseMove(event);
        wasHovered_ = hovered_;
        return result;
    }
    
    bool handleMouseButton(const MouseEvent& event) override {
        pressed_ = event.pressed && bounds_.contains(event.position);
        return Widget::handleMouseButton(event);
    }
    
private:
    std::string label_;
    std::string iconPath_;
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
        style_.borderColor = Color(0.3f, 0.3f, 0.3f, 1.0f);
        style_.borderWidth = 1;
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
    TextInput& onSubmit(std::function<void(const std::string&)> handler) {
        onSubmitHandler_ = std::move(handler);
        return *this;
    }
    
    const std::string& value() const { return text_; }
    
    Size measureContent(Size available) override {
        return Size(200, textStyle_.fontSize * 1.5f);
    }
    
    void render(Renderer& renderer) override {
        // Change border color when focused
        if (focused_) {
            style_.borderColor = Color::fromHex(0x3b82f6ff);
            style_.borderWidth = 2;
        } else {
            style_.borderColor = Color(0.3f, 0.3f, 0.3f, 1.0f);
            style_.borderWidth = 1;
        }
        
        Widget::render(renderer);
        
        Font* font = renderer.loadFont(textStyle_.fontFamily, textStyle_.fontSize);
        if (!font) return;
        
        std::string displayText = text_.empty() ? placeholder_ : text_;
        Color displayColor = text_.empty() ? 
            textStyle_.color.withAlpha(0.5f) : textStyle_.color;
        
        renderer.drawText(displayText, contentBounds_.topLeft(), font, 
                         displayColor, textStyle_);
        
        // Draw cursor
        if (focused_) {
            Size textSize = font->measureText(text_.substr(0, cursorPos_));
            Rect cursor(
                contentBounds_.x + textSize.width,
                contentBounds_.y + 2,
                2,
                contentBounds_.height - 4
            );
            renderer.drawRect(cursor, textStyle_.color);
        }
    }
    
    bool handleKeyEvent(const KeyEvent& event) override {
        if (!focused_) return false;
        
        if (event.pressed) {
            if (event.keycode == 14) {  // Backspace
                if (cursorPos_ > 0) {
                    text_.erase(cursorPos_ - 1, 1);
                    cursorPos_--;
                    if (onChangeHandler_) onChangeHandler_(text_);
                }
            } else if (event.keycode == 111) {  // Delete
                if (cursorPos_ < text_.length()) {
                    text_.erase(cursorPos_, 1);
                    if (onChangeHandler_) onChangeHandler_(text_);
                }
            } else if (event.keycode == 105) {  // Left
                if (cursorPos_ > 0) cursorPos_--;
            } else if (event.keycode == 106) {  // Right
                if (cursorPos_ < text_.length()) cursorPos_++;
            } else if (event.keycode == 28) {  // Enter
                if (onSubmitHandler_) onSubmitHandler_(text_);
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
    std::function<void(const std::string&)> onSubmitHandler_;
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
    
    void render(Renderer& renderer) override {
        Widget::render(renderer);
        
        float fillWidth = (value_ - min_) / (max_ - min_) * contentBounds_.width;
        Rect fillRect(contentBounds_.x, contentBounds_.y, fillWidth, contentBounds_.height);
        renderer.drawRoundedRect(fillRect, style_.borderRadius, fillColor_);
        
        float thumbX = contentBounds_.x + fillWidth - 8;
        float thumbY = contentBounds_.y - 5;
        Rect thumbRect(thumbX, thumbY, 16, contentBounds_.height + 10);
        renderer.drawRoundedRect(thumbRect, BorderRadius(8), thumbColor_);
    }
    
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
        if (onChangeHandler_) onChangeHandler_(value_);
    }
};

// ============================================================================
// Checkbox Widget
// ============================================================================

class Checkbox : public Widget {
public:
    explicit Checkbox(bool checked = false) : checked_(checked) {
        style_.background = Color(0.2f, 0.2f, 0.2f, 1.0f);
        style_.borderColor = Color(0.4f, 0.4f, 0.4f, 1.0f);
        style_.borderWidth = 1;
        style_.borderRadius = BorderRadius(3);
    }
    
    Checkbox& checked(bool c) { checked_ = c; return *this; }
    Checkbox& onToggle(std::function<void(bool)> handler) {
        onToggleHandler_ = std::move(handler);
        return *this;
    }
    Checkbox& checkColor(const Color& c) { checkColor_ = c; return *this; }
    
    bool isChecked() const { return checked_; }
    
    Size measureContent(Size available) override {
        return Size(20, 20);
    }
    
    void render(Renderer& renderer) override {
        if (checked_) {
            style_.background = Color::fromHex(0x3b82f6ff);
        } else {
            style_.background = Color(0.2f, 0.2f, 0.2f, 1.0f);
        }
        
        Widget::render(renderer);
        
        if (checked_) {
            // Draw checkmark
            float cx = contentBounds_.x + contentBounds_.width / 2;
            float cy = contentBounds_.y + contentBounds_.height / 2;
            
            // Simple cross check
            renderer.drawRect(
                Rect(cx - 5, cy - 1, 10, 2),
                checkColor_
            );
            renderer.drawRect(
                Rect(cx - 1, cy - 5, 2, 10),
                checkColor_
            );
        }
    }
    
    bool handleMouseButton(const MouseEvent& event) override {
        if (event.pressed && event.button == MouseButton::Left) {
            if (bounds_.contains(event.position)) {
                checked_ = !checked_;
                if (onToggleHandler_) onToggleHandler_(checked_);
                return true;
            }
        }
        return false;
    }
    
private:
    bool checked_;
    Color checkColor_ = Color(1, 1, 1, 1);
    std::function<void(bool)> onToggleHandler_;
};

// ============================================================================
// ProgressBar Widget
// ============================================================================

class ProgressBar : public Widget {
public:
    explicit ProgressBar(float progress = 0.0f) : progress_(progress) {
        style_.background = Color(0.2f, 0.2f, 0.2f, 1.0f);
        style_.borderRadius = BorderRadius(3);
    }
    
    ProgressBar& progress(float p) { progress_ = std::clamp(p, 0.0f, 1.0f); return *this; }
    ProgressBar& fillColor(const Color& c) { fillColor_ = c; return *this; }
    ProgressBar& showText(bool s) { showText_ = s; return *this; }
    
    float getProgress() const { return progress_; }
    
    Size measureContent(Size available) override {
        return Size(200, 10);
    }
    
    void render(Renderer& renderer) override {
        Widget::render(renderer);
        
        float fillWidth = progress_ * contentBounds_.width;
        Rect fillRect(contentBounds_.x, contentBounds_.y, fillWidth, contentBounds_.height);
        renderer.drawRoundedRect(fillRect, style_.borderRadius, fillColor_);
        
        if (showText_ && contentBounds_.height >= 14) {
            std::string text = std::to_string((int)(progress_ * 100)) + "%";
            Font* font = renderer.loadFont("", 10);
            if (font) {
                Size textSize = font->measureText(text);
                Point textPos(
                    contentBounds_.x + (contentBounds_.width - textSize.width) / 2,
                    contentBounds_.y + (contentBounds_.height - textSize.height) / 2
                );
                renderer.drawText(text, textPos, font, Color(1, 1, 1, 1), TextStyle());
            }
        }
    }
    
private:
    float progress_;
    Color fillColor_ = Color::fromHex(0x3b82f6ff);
    bool showText_ = false;
};

// ============================================================================
// Spacer Widget
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
    
    void render(Renderer&) override {}
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
    
    Divider& color(const Color& c) { style_.background = c; return *this; }
    Divider& thickness(float t) {
        if (direction_ == Direction::Horizontal) {
            heightSpec_ = SizeSpec::fixed(t);
        } else {
            widthSpec_ = SizeSpec::fixed(t);
        }
        return *this;
    }
    
private:
    Direction direction_;
};

// ============================================================================
// Label (Text with optional icon)
// ============================================================================

class Label : public Widget {
public:
    explicit Label(const std::string& text = "") : text_(text) {
        textStyle_.fontSize = 14;
        textStyle_.color = Color(1, 1, 1, 1);
    }
    
    Label& text(const std::string& t) { text_ = t; return *this; }
    Label& fontSize(float s) { textStyle_.fontSize = s; return *this; }
    Label& color(const Color& c) { textStyle_.color = c; return *this; }
    Label& bold(bool b) { textStyle_.bold = b; return *this; }
    
    Size measureContent(Size available) override {
        return Size(text_.length() * textStyle_.fontSize * 0.6f, 
                    textStyle_.fontSize * 1.4f);
    }
    
    void render(Renderer& renderer) override {
        Widget::render(renderer);
        
        Font* font = renderer.loadFont(textStyle_.fontFamily, textStyle_.fontSize);
        if (font) {
            renderer.drawText(text_, contentBounds_.topLeft(), font, 
                            textStyle_.color, textStyle_);
        }
    }
    
private:
    std::string text_;
    TextStyle textStyle_;
};

} // namespace MetaUI
