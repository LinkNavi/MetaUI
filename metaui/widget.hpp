#pragma once

#include "core.hpp"
#include <memory>
#include <vector>
#include <functional>

namespace MetaUI {

// Forward declarations
class Renderer;
class Widget;

using WidgetPtr = std::shared_ptr<Widget>;

// ============================================================================
// Widget Base Class
// ============================================================================

class Widget : public std::enable_shared_from_this<Widget> {
public:
    Widget() = default;
    virtual ~Widget() = default;
    
    // Layout
    Widget& width(SizeSpec spec) { widthSpec_ = spec; return *this; }
    Widget& height(SizeSpec spec) { heightSpec_ = spec; return *this; }
    Widget& size(SizeSpec w, SizeSpec h) { widthSpec_ = w; heightSpec_ = h; return *this; }
    
    // Styling
    Widget& padding(const Padding& p) { style_.padding = p; return *this; }
    Widget& padding(float all) { style_.padding = Padding(all); return *this; }
    Widget& margin(const Padding& m) { style_.margin = m; return *this; }
    Widget& margin(float all) { style_.margin = Padding(all); return *this; }
    
    Widget& background(const Color& c) { style_.background = c; return *this; }
    Widget& border(const Color& c, float width = 1.0f) { 
        style_.borderColor = c; 
        style_.borderWidth = width; 
        return *this; 
    }
    Widget& borderRadius(float r) { style_.borderRadius = BorderRadius(r); return *this; }
    Widget& borderRadius(float tl, float tr, float br, float bl) {
        style_.borderRadius = BorderRadius(tl, tr, br, bl);
        return *this;
    }
    
    Widget& shadow(const Color& c, Point offset = Point(0, 2), float blur = 4.0f) {
        style_.hasShadow = true;
        style_.shadowColor = c;
        style_.shadowOffset = offset;
        style_.shadowBlur = blur;
        return *this;
    }
    
    Widget& gradient(const Color& start, const Color& end, float angle = 90.0f) {
        style_.hasGradient = true;
        style_.gradientStart = start;
        style_.gradientEnd = end;
        style_.gradientAngle = angle;
        return *this;
    }
    
    // Event handlers
    Widget& onClick(std::function<void()> handler) {
        onClickHandler_ = std::move(handler);
        return *this;
    }
    
    Widget& onHover(std::function<void(bool)> handler) {
        onHoverHandler_ = std::move(handler);
        return *this;
    }
    
    Widget& onFocus(std::function<void(bool)> handler) {
        onFocusHandler_ = std::move(handler);
        return *this;
    }
    
    // Visibility
    Widget& visible(bool v) { visible_ = v; return *this; }
    Widget& enabled(bool e) { enabled_ = e; return *this; }
    
    // Getters
    const Rect& bounds() const { return bounds_; }
    const BoxStyle& style() const { return style_; }
    bool isVisible() const { return visible_; }
    bool isEnabled() const { return enabled_; }
    bool isHovered() const { return hovered_; }
    bool isFocused() const { return focused_; }
    
    // Layout calculation
    virtual Size measureContent(Size available) { return Size(0, 0); }
    
    Size measure(Size available) {
        if (!visible_) return Size(0, 0);
        
        // Apply margin
        available.width -= style_.margin.horizontal();
        available.height -= style_.margin.vertical();
        
        // Calculate size based on constraints
        Size result;
        
        // Width
        switch (widthSpec_.constraint) {
            case SizeConstraint::Fixed:
                result.width = widthSpec_.value;
                break;
            case SizeConstraint::Fill:
                result.width = available.width;
                break;
            case SizeConstraint::Percent:
                result.width = available.width * (widthSpec_.value / 100.0f);
                break;
            case SizeConstraint::Content:
                result.width = measureContent(available).width + style_.padding.horizontal();
                break;
        }
        
        // Height
        switch (heightSpec_.constraint) {
            case SizeConstraint::Fixed:
                result.height = heightSpec_.value;
                break;
            case SizeConstraint::Fill:
                result.height = available.height;
                break;
            case SizeConstraint::Percent:
                result.height = available.height * (heightSpec_.value / 100.0f);
                break;
            case SizeConstraint::Content:
                result.height = measureContent(available).height + style_.padding.vertical();
                break;
        }
        
        measuredSize_ = result;
        return result;
    }
    
    void layout(const Rect& rect) {
        bounds_ = rect;
        
        // Content bounds (excluding padding)
        contentBounds_ = Rect(
            rect.x + style_.padding.left,
            rect.y + style_.padding.top,
            rect.width - style_.padding.horizontal(),
            rect.height - style_.padding.vertical()
        );
        
        layoutChildren();
    }
    
    virtual void layoutChildren() {}
    
    // Rendering
    virtual void render(Renderer& renderer);
    
    // Event handling
    virtual bool handleMouseMove(const MouseEvent& event) {
        bool wasHovered = hovered_;
        hovered_ = bounds_.contains(event.position);
        
        if (hovered_ != wasHovered && onHoverHandler_) {
            onHoverHandler_(hovered_);
        }
        
        return hovered_;
    }
    
    virtual bool handleMouseButton(const MouseEvent& event) {
        if (!enabled_) return false;
        
        if (event.pressed && event.button == MouseButton::Left) {
            if (bounds_.contains(event.position)) {
                if (onClickHandler_) {
                    onClickHandler_();
                }
                return true;
            }
        }
        return false;
    }
    
    virtual bool handleKeyEvent(const KeyEvent& event) {
        return false;
    }
    
    virtual bool handleScroll(const ScrollEvent& event) {
        return false;
    }
    
    void setFocus(bool focus) {
        if (focused_ != focus) {
            focused_ = focus;
            if (onFocusHandler_) {
                onFocusHandler_(focused_);
            }
        }
    }
    
protected:
    // Layout specs
    SizeSpec widthSpec_{SizeConstraint::Content};
    SizeSpec heightSpec_{SizeConstraint::Content};
    
    // Style
    BoxStyle style_;
    
    // Bounds
    Rect bounds_;
    Rect contentBounds_;
    Size measuredSize_;
    
    // State
    bool visible_ = true;
    bool enabled_ = true;
    bool hovered_ = false;
    bool focused_ = false;
    
    // Event handlers
    std::function<void()> onClickHandler_;
    std::function<void(bool)> onHoverHandler_;
    std::function<void(bool)> onFocusHandler_;
};

// ============================================================================
// Container Widget
// ============================================================================

class Container : public Widget {
public:
    Container& addChild(WidgetPtr child) {
        children_.push_back(std::move(child));
        return *this;
    }
    
    Container& clearChildren() {
        children_.clear();
        return *this;
    }
    
    const std::vector<WidgetPtr>& children() const { return children_; }
    
    void render(Renderer& renderer) override {
        Widget::render(renderer);
        
        for (auto& child : children_) {
            if (child->isVisible()) {
                child->render(renderer);
            }
        }
    }
    
    bool handleMouseMove(const MouseEvent& event) override {
        Widget::handleMouseMove(event);
        
        for (auto& child : children_) {
            if (child->handleMouseMove(event)) {
                return true;
            }
        }
        return false;
    }
    
    bool handleMouseButton(const MouseEvent& event) override {
        if (Widget::handleMouseButton(event)) return true;
        
        for (auto& child : children_) {
            if (child->handleMouseButton(event)) {
                return true;
            }
        }
        return false;
    }
    
protected:
    std::vector<WidgetPtr> children_;
};

} // namespace MetaUI
