#pragma once

#include "widget.hpp"
#include <algorithm>

namespace MetaUI {

// ============================================================================
// Box Layout (Horizontal/Vertical)
// ============================================================================

class Box : public Container {
public:
    explicit Box(Direction dir = Direction::Horizontal) : direction_(dir) {}
    
    Box& direction(Direction dir) { direction_ = dir; return *this; }
    Box& spacing(float s) { spacing_ = s; return *this; }
    Box& align(Alignment a) { alignment_ = a; return *this; }
    Box& crossAlign(Alignment a) { crossAlignment_ = a; return *this; }
    
    Size measureContent(Size available) override {
        if (children_.empty()) return Size(0, 0);
        
        Size result(0, 0);
        Size childAvailable = available;
        float totalSpacing = spacing_ * (children_.size() - 1);
        
        if (direction_ == Direction::Horizontal) {
            childAvailable.width -= totalSpacing;
            for (auto& child : children_) {
                Size childSize = child->measure(childAvailable);
                result.width += childSize.width;
                result.height = std::max(result.height, childSize.height);
            }
            result.width += totalSpacing;
        } else {
            childAvailable.height -= totalSpacing;
            for (auto& child : children_) {
                Size childSize = child->measure(childAvailable);
                result.height += childSize.height;
                result.width = std::max(result.width, childSize.width);
            }
            result.height += totalSpacing;
        }
        return result;
    }
    
    void layoutChildren() override {
        if (children_.empty()) return;
        
        std::vector<Size> childSizes;
        for (auto& child : children_) {
            childSizes.push_back(child->measure(Size(contentBounds_.width, contentBounds_.height)));
        }
        
        float totalSpacing = spacing_ * (children_.size() - 1);
        
        if (direction_ == Direction::Horizontal) {
            layoutHorizontal(childSizes, totalSpacing);
        } else {
            layoutVertical(childSizes, totalSpacing);
        }
    }
    
private:
    Direction direction_;
    float spacing_ = 0;
    Alignment alignment_ = Alignment::Start;
    Alignment crossAlignment_ = Alignment::Start;
    
    void layoutHorizontal(const std::vector<Size>& sizes, float totalSpacing) {
        float totalWidth = 0;
        for (const auto& size : sizes) totalWidth += size.width;
        totalWidth += totalSpacing;
        
        float x = contentBounds_.x;
        if (alignment_ == Alignment::Center) x += (contentBounds_.width - totalWidth) / 2;
        else if (alignment_ == Alignment::End) x += contentBounds_.width - totalWidth;
        
        for (size_t i = 0; i < children_.size(); ++i) {
            float y = contentBounds_.y;
            float height = sizes[i].height;
            
            if (crossAlignment_ == Alignment::Center) y += (contentBounds_.height - height) / 2;
            else if (crossAlignment_ == Alignment::End) y += contentBounds_.height - height;
            else if (crossAlignment_ == Alignment::Stretch) height = contentBounds_.height;
            
            children_[i]->layout(Rect(x, y, sizes[i].width, height));
            x += sizes[i].width + spacing_;
        }
    }
    
    void layoutVertical(const std::vector<Size>& sizes, float totalSpacing) {
        float totalHeight = 0;
        for (const auto& size : sizes) totalHeight += size.height;
        totalHeight += totalSpacing;
        
        float y = contentBounds_.y;
        if (alignment_ == Alignment::Center) y += (contentBounds_.height - totalHeight) / 2;
        else if (alignment_ == Alignment::End) y += contentBounds_.height - totalHeight;
        
        for (size_t i = 0; i < children_.size(); ++i) {
            float x = contentBounds_.x;
            float width = sizes[i].width;
            
            if (crossAlignment_ == Alignment::Center) x += (contentBounds_.width - width) / 2;
            else if (crossAlignment_ == Alignment::End) x += contentBounds_.width - width;
            else if (crossAlignment_ == Alignment::Stretch) width = contentBounds_.width;
            
            children_[i]->layout(Rect(x, y, width, sizes[i].height));
            y += sizes[i].height + spacing_;
        }
    }
};

// ============================================================================
// Stack Layout
// ============================================================================

class Stack : public Container {
public:
    Stack& align(Alignment h, Alignment v) {
        horizontalAlign_ = h;
        verticalAlign_ = v;
        return *this;
    }
    
    Size measureContent(Size available) override {
        Size result(0, 0);
        for (auto& child : children_) {
            Size childSize = child->measure(available);
            result.width = std::max(result.width, childSize.width);
            result.height = std::max(result.height, childSize.height);
        }
        return result;
    }
    
    void layoutChildren() override {
        for (auto& child : children_) {
            Size childSize = child->measure(Size(contentBounds_.width, contentBounds_.height));
            
            float x = contentBounds_.x;
            float y = contentBounds_.y;
            
            if (horizontalAlign_ == Alignment::Center) x += (contentBounds_.width - childSize.width) / 2;
            else if (horizontalAlign_ == Alignment::End) x += contentBounds_.width - childSize.width;
            
            if (verticalAlign_ == Alignment::Center) y += (contentBounds_.height - childSize.height) / 2;
            else if (verticalAlign_ == Alignment::End) y += contentBounds_.height - childSize.height;
            
            child->layout(Rect(x, y, childSize.width, childSize.height));
        }
    }
    
private:
    Alignment horizontalAlign_ = Alignment::Start;
    Alignment verticalAlign_ = Alignment::Start;
};

// ============================================================================
// Grid Layout
// ============================================================================

class Grid : public Container {
public:
    Grid& columns(int cols) { columns_ = cols; return *this; }
    Grid& spacing(float s) { spacing_ = s; return *this; }
    Grid& cellSize(Size s) { cellSize_ = s; return *this; }
    
    Size measureContent(Size available) override {
        if (children_.empty() || columns_ <= 0) return Size(0, 0);
        
        int rows = (children_.size() + columns_ - 1) / columns_;
        float cellWidth = cellSize_.width > 0 ? cellSize_.width : 
            (available.width - spacing_ * (columns_ - 1)) / columns_;
        float cellHeight = cellSize_.height > 0 ? cellSize_.height : cellWidth;
        
        return Size(
            cellWidth * columns_ + spacing_ * (columns_ - 1),
            cellHeight * rows + spacing_ * (rows - 1)
        );
    }
    
    void layoutChildren() override {
        if (children_.empty() || columns_ <= 0) return;
        
        float cellWidth = cellSize_.width > 0 ? cellSize_.width :
            (contentBounds_.width - spacing_ * (columns_ - 1)) / columns_;
        float cellHeight = cellSize_.height > 0 ? cellSize_.height : cellWidth;
        
        int row = 0, col = 0;
        for (auto& child : children_) {
            float x = contentBounds_.x + col * (cellWidth + spacing_);
            float y = contentBounds_.y + row * (cellHeight + spacing_);
            
            child->layout(Rect(x, y, cellWidth, cellHeight));
            
            if (++col >= columns_) { col = 0; row++; }
        }
    }
    
private:
    int columns_ = 1;
    float spacing_ = 0;
    Size cellSize_{0, 0};
};

// ============================================================================
// ScrollView
// ============================================================================

class ScrollView : public Container {
public:
    ScrollView& scrollDirection(Direction dir) { scrollDir_ = dir; return *this; }
    
    Size measureContent(Size available) override {
        if (children_.empty()) return Size(0, 0);
        if (scrollDir_ == Direction::Horizontal) {
            return Size(children_[0]->measure(Size(1e9f, available.height)).width, available.height);
        }
        return Size(available.width, children_[0]->measure(Size(available.width, 1e9f)).height);
    }
    
    void layoutChildren() override {
        if (children_.empty()) return;
        
        Size childSize = children_[0]->measure(Size(
            scrollDir_ == Direction::Horizontal ? 1e9f : contentBounds_.width,
            scrollDir_ == Direction::Vertical ? 1e9f : contentBounds_.height
        ));
        
        children_[0]->layout(Rect(
            contentBounds_.x - scrollOffset_.x,
            contentBounds_.y - scrollOffset_.y,
            childSize.width,
            childSize.height
        ));
    }
    
    bool handleScroll(const ScrollEvent& event) override {
        if (!contentBounds_.contains(event.position)) return false;
        
        if (scrollDir_ == Direction::Horizontal) {
            scrollOffset_.x -= event.deltaX * 20;
            scrollOffset_.x = std::max(0.0f, scrollOffset_.x);
        } else {
            scrollOffset_.y -= event.deltaY * 20;
            scrollOffset_.y = std::max(0.0f, scrollOffset_.y);
        }
        layoutChildren();
        return true;
    }
    
private:
    Direction scrollDir_ = Direction::Vertical;
    Point scrollOffset_;
};

// ============================================================================
// Sidebar Layout
// ============================================================================

class Sidebar : public Container {
public:
    enum class Position { Left, Right, Top, Bottom };
    
    Sidebar(Position pos = Position::Left, float size = 200)
        : position_(pos), sidebarSize_(size) {}
    
    Sidebar& position(Position pos) { position_ = pos; return *this; }
    Sidebar& sidebarSize(float size) { sidebarSize_ = size; return *this; }
    
    void layoutChildren() override {
        if (children_.size() < 2) return;
        
        auto& sidebar = children_[0];
        auto& content = children_[1];
        
        switch (position_) {
            case Position::Left:
                sidebar->layout(Rect(contentBounds_.x, contentBounds_.y, sidebarSize_, contentBounds_.height));
                content->layout(Rect(contentBounds_.x + sidebarSize_, contentBounds_.y,
                    contentBounds_.width - sidebarSize_, contentBounds_.height));
                break;
            case Position::Right:
                content->layout(Rect(contentBounds_.x, contentBounds_.y,
                    contentBounds_.width - sidebarSize_, contentBounds_.height));
                sidebar->layout(Rect(contentBounds_.x + contentBounds_.width - sidebarSize_, 
                    contentBounds_.y, sidebarSize_, contentBounds_.height));
                break;
            case Position::Top:
                sidebar->layout(Rect(contentBounds_.x, contentBounds_.y, contentBounds_.width, sidebarSize_));
                content->layout(Rect(contentBounds_.x, contentBounds_.y + sidebarSize_,
                    contentBounds_.width, contentBounds_.height - sidebarSize_));
                break;
            case Position::Bottom:
                content->layout(Rect(contentBounds_.x, contentBounds_.y,
                    contentBounds_.width, contentBounds_.height - sidebarSize_));
                sidebar->layout(Rect(contentBounds_.x, contentBounds_.y + contentBounds_.height - sidebarSize_,
                    contentBounds_.width, sidebarSize_));
                break;
        }
    }
    
private:
    Position position_;
    float sidebarSize_;
};

} // namespace MetaUI
