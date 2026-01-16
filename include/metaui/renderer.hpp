#pragma once

#include "core.hpp"
#include "widget.hpp"
#include <GL/gl.h>
#include <unordered_map>
#include <vector>

// Font rendering with stb_truetype (header-only)
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace MetaUI {

// ============================================================================
// Texture Management
// ============================================================================

class Texture {
public:
    Texture(int width, int height, const unsigned char* data) 
        : width_(width), height_(height) {
        glGenTextures(1, &id_);
        glBindTexture(GL_TEXTURE_2D, id_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
                     GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    
    ~Texture() {
        if (id_) glDeleteTextures(1, &id_);
    }
    
    GLuint id() const { return id_; }
    int width() const { return width_; }
    int height() const { return height_; }
    
private:
    GLuint id_ = 0;
    int width_, height_;
};

// ============================================================================
// Font Management
// ============================================================================

class Font {
public:
    Font(const std::string& path, float size) : size_(size) {
        FILE* file = fopen(path.c_str(), "rb");
        if (!file) return;
        
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        fontData_.resize(fileSize);
        fread(fontData_.data(), 1, fileSize, file);
        fclose(file);
        
        stbtt_InitFont(&font_, fontData_.data(), 0);
        scale_ = stbtt_ScaleForPixelHeight(&font_, size);
        
        // Create bitmap atlas for common ASCII characters
        createAtlas();
    }
    
    struct GlyphInfo {
        float u0, v0, u1, v1;  // Texture coordinates
        float x0, y0, x1, y1;  // Glyph bounds
        float advance;
    };
    
    const GlyphInfo* getGlyph(int codepoint) const {
        auto it = glyphs_.find(codepoint);
        return it != glyphs_.end() ? &it->second : nullptr;
    }
    
    GLuint atlasTexture() const { return atlasTexture_; }
    
    Size measureText(const std::string& text) const {
        float width = 0;
        float height = size_;
        
        for (char c : text) {
            auto* glyph = getGlyph(c);
            if (glyph) {
                width += glyph->advance;
            }
        }
        
        return Size(width, height);
    }
    
private:
    stbtt_fontinfo font_;
    std::vector<unsigned char> fontData_;
    float size_;
    float scale_;
    std::unordered_map<int, GlyphInfo> glyphs_;
    GLuint atlasTexture_ = 0;
    
    void createAtlas() {
        const int atlasWidth = 512;
        const int atlasHeight = 512;
        std::vector<unsigned char> atlasData(atlasWidth * atlasHeight, 0);
        
        int x = 0, y = 0;
        int rowHeight = 0;
        
        // Bake ASCII characters
        for (int c = 32; c < 128; ++c) {
            int width, height, xoff, yoff;
            unsigned char* bitmap = stbtt_GetCodepointBitmap(
                &font_, 0, scale_, c, &width, &height, &xoff, &yoff);
            
            if (!bitmap) continue;
            
            // Check if we need a new row
            if (x + width >= atlasWidth) {
                x = 0;
                y += rowHeight;
                rowHeight = 0;
            }
            
            if (y + height >= atlasHeight) {
                stbtt_FreeBitmap(bitmap, nullptr);
                break;
            }
            
            // Copy bitmap to atlas
            for (int row = 0; row < height; ++row) {
                for (int col = 0; col < width; ++col) {
                    atlasData[(y + row) * atlasWidth + (x + col)] = 
                        bitmap[row * width + col];
                }
            }
            
            // Get advance
            int advance;
            stbtt_GetCodepointHMetrics(&font_, c, &advance, nullptr);
            
            // Store glyph info
            GlyphInfo info;
            info.u0 = (float)x / atlasWidth;
            info.v0 = (float)y / atlasHeight;
            info.u1 = (float)(x + width) / atlasWidth;
            info.v1 = (float)(y + height) / atlasHeight;
            info.x0 = xoff;
            info.y0 = yoff;
            info.x1 = xoff + width;
            info.y1 = yoff + height;
            info.advance = advance * scale_;
            
            glyphs_[c] = info;
            
            x += width + 1;
            rowHeight = std::max(rowHeight, height + 1);
            
            stbtt_FreeBitmap(bitmap, nullptr);
        }
        
        // Create OpenGL texture
        glGenTextures(1, &atlasTexture_);
        glBindTexture(GL_TEXTURE_2D, atlasTexture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, atlasWidth, atlasHeight, 0,
                     GL_ALPHA, GL_UNSIGNED_BYTE, atlasData.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
};

// ============================================================================
// OpenGL Renderer
// ============================================================================

class Renderer {
public:
    Renderer(int width, int height) : width_(width), height_(height) {
        glViewport(0, 0, width, height);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    
    void setSize(int width, int height) {
        width_ = width;
        height_ = height;
        glViewport(0, 0, width, height);
    }
    
    void beginFrame() {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width_, height_, 0, -1, 1);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    
    void endFrame() {
        glFlush();
    }
    
    // Draw primitives
    void drawRect(const Rect& rect, const Color& color) {
        glColor4f(color.r, color.g, color.b, color.a);
        glBegin(GL_QUADS);
        glVertex2f(rect.x, rect.y);
        glVertex2f(rect.x + rect.width, rect.y);
        glVertex2f(rect.x + rect.width, rect.y + rect.height);
        glVertex2f(rect.x, rect.y + rect.height);
        glEnd();
    }
    
    void drawRoundedRect(const Rect& rect, const BorderRadius& radius, const Color& color) {
        // Simple rounded rect using triangle fan for corners
        glColor4f(color.r, color.g, color.b, color.a);
        
        // Center rectangle
        drawRect(Rect(rect.x + radius.topLeft, rect.y, 
                     rect.width - radius.topLeft - radius.topRight, radius.topLeft), color);
        drawRect(Rect(rect.x, rect.y + radius.topLeft,
                     rect.width, rect.height - radius.topLeft - radius.bottomLeft), color);
        drawRect(Rect(rect.x + radius.bottomLeft, rect.y + rect.height - radius.bottomLeft,
                     rect.width - radius.bottomLeft - radius.bottomRight, radius.bottomLeft), color);
        
        // Draw corners (simplified - would use proper circles in production)
        drawCorner(rect.x + radius.topLeft, rect.y + radius.topLeft, 
                  radius.topLeft, 180, 270, color);
        drawCorner(rect.x + rect.width - radius.topRight, rect.y + radius.topRight,
                  radius.topRight, 270, 360, color);
        drawCorner(rect.x + rect.width - radius.bottomRight, 
                  rect.y + rect.height - radius.bottomRight,
                  radius.bottomRight, 0, 90, color);
        drawCorner(rect.x + radius.bottomLeft, rect.y + rect.height - radius.bottomLeft,
                  radius.bottomLeft, 90, 180, color);
    }
    
    void drawBorder(const Rect& rect, const BorderRadius& radius, 
                   const Color& color, float width) {
        glLineWidth(width);
        glColor4f(color.r, color.g, color.b, color.a);
        
        // Draw border lines and rounded corners
        // Simplified version - full version would properly handle rounded corners
        glBegin(GL_LINE_LOOP);
        glVertex2f(rect.x + radius.topLeft, rect.y);
        glVertex2f(rect.x + rect.width - radius.topRight, rect.y);
        glVertex2f(rect.x + rect.width, rect.y + radius.topRight);
        glVertex2f(rect.x + rect.width, rect.y + rect.height - radius.bottomRight);
        glVertex2f(rect.x + rect.width - radius.bottomRight, rect.y + rect.height);
        glVertex2f(rect.x + radius.bottomLeft, rect.y + rect.height);
        glVertex2f(rect.x, rect.y + rect.height - radius.bottomLeft);
        glVertex2f(rect.x, rect.y + radius.topLeft);
        glEnd();
    }
    
    void drawGradient(const Rect& rect, const Color& start, const Color& end, float angle) {
        // Vertical gradient (simplified)
        glBegin(GL_QUADS);
        glColor4f(start.r, start.g, start.b, start.a);
        glVertex2f(rect.x, rect.y);
        glVertex2f(rect.x + rect.width, rect.y);
        glColor4f(end.r, end.g, end.b, end.a);
        glVertex2f(rect.x + rect.width, rect.y + rect.height);
        glVertex2f(rect.x, rect.y + rect.height);
        glEnd();
    }
    
    void drawText(const std::string& text, const Point& pos, Font* font, 
                 const Color& color, const TextStyle& style) {
        if (!font) return;
        
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, font->atlasTexture());
        glColor4f(color.r, color.g, color.b, color.a);
        
        float x = pos.x;
        float y = pos.y;
        
        glBegin(GL_QUADS);
        for (char c : text) {
            auto* glyph = font->getGlyph(c);
            if (!glyph) continue;
            
            float x0 = x + glyph->x0;
            float y0 = y + glyph->y0;
            float x1 = x + glyph->x1;
            float y1 = y + glyph->y1;
            
            glTexCoord2f(glyph->u0, glyph->v0); glVertex2f(x0, y0);
            glTexCoord2f(glyph->u1, glyph->v0); glVertex2f(x1, y0);
            glTexCoord2f(glyph->u1, glyph->v1); glVertex2f(x1, y1);
            glTexCoord2f(glyph->u0, glyph->v1); glVertex2f(x0, y1);
            
            x += glyph->advance;
        }
        glEnd();
        
        glDisable(GL_TEXTURE_2D);
    }
    
    void drawImage(const Texture& texture, const Rect& rect) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture.id());
        glColor4f(1, 1, 1, 1);
        
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(rect.x, rect.y);
        glTexCoord2f(1, 0); glVertex2f(rect.x + rect.width, rect.y);
        glTexCoord2f(1, 1); glVertex2f(rect.x + rect.width, rect.y + rect.height);
        glTexCoord2f(0, 1); glVertex2f(rect.x, rect.y + rect.height);
        glEnd();
        
        glDisable(GL_TEXTURE_2D);
    }
    
    Font* loadFont(const std::string& path, float size) {
        std::string key = path + ":" + std::to_string(size);
        auto it = fonts_.find(key);
        if (it != fonts_.end()) {
            return it->second.get();
        }
        
        auto font = std::make_unique<Font>(path, size);
        Font* ptr = font.get();
        fonts_[key] = std::move(font);
        return ptr;
    }
    
private:
    int width_, height_;
    std::unordered_map<std::string, std::unique_ptr<Font>> fonts_;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures_;
    
    void drawCorner(float cx, float cy, float radius, float startAngle, float endAngle, 
                   const Color& color) {
        const int segments = 16;
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int i = 0; i <= segments; ++i) {
            float angle = startAngle + (endAngle - startAngle) * i / segments;
            float rad = angle * 3.14159f / 180.0f;
            glVertex2f(cx + cos(rad) * radius, cy + sin(rad) * radius);
        }
        glEnd();
    }
};

// ============================================================================
// Widget Rendering Implementations
// ============================================================================

inline void Widget::render(Renderer& renderer) {
    if (!visible_) return;
    
    // Draw shadow
    if (style_.hasShadow) {
        Rect shadowRect = bounds_;
        shadowRect.x += style_.shadowOffset.x;
        shadowRect.y += style_.shadowOffset.y;
        renderer.drawRoundedRect(shadowRect, style_.borderRadius, style_.shadowColor);
    }
    
    // Draw background
    if (style_.hasGradient) {
        renderer.drawGradient(bounds_, style_.gradientStart, style_.gradientEnd, 
                            style_.gradientAngle);
    } else if (style_.background.a > 0) {
        if (style_.borderRadius.topLeft > 0 || style_.borderRadius.topRight > 0 ||
            style_.borderRadius.bottomLeft > 0 || style_.borderRadius.bottomRight > 0) {
            renderer.drawRoundedRect(bounds_, style_.borderRadius, style_.background);
        } else {
            renderer.drawRect(bounds_, style_.background);
        }
    }
    
    // Draw border
    if (style_.borderWidth > 0 && style_.borderColor.a > 0) {
        renderer.drawBorder(bounds_, style_.borderRadius, style_.borderColor, 
                          style_.borderWidth);
    }
}

} // namespace MetaUI
