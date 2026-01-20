#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#ifndef STB_IMAGE_INCLUDED
#define STB_IMAGE_INCLUDED
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#include "stb_image.h"
#endif

#pragma once

#include "core.hpp"
#include "widget.hpp"
#include <GL/gl.h>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <algorithm>



#ifndef STB_IMAGE_INCLUDED
#define STB_IMAGE_INCLUDED
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#include "stb_image.h"
#endif

namespace MetaUI {

// ============================================================================
// Texture Management
// ============================================================================

class Texture {
public:
    Texture() = default;
    
    Texture(int width, int height, const unsigned char* data, int channels = 4) 
        : width_(width), height_(height) {
        glGenTextures(1, &id_);
        glBindTexture(GL_TEXTURE_2D, id_);
        
        GLenum format = (channels == 4) ? GL_RGBA : 
                        (channels == 3) ? GL_RGB : 
                        (channels == 1) ? GL_ALPHA : GL_RGBA;
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, 
                     format, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    ~Texture() {
        if (id_) glDeleteTextures(1, &id_);
    }
    
    // Move only
    Texture(Texture&& other) noexcept 
        : id_(other.id_), width_(other.width_), height_(other.height_) {
        other.id_ = 0;
    }
    
    Texture& operator=(Texture&& other) noexcept {
        if (this != &other) {
            if (id_) glDeleteTextures(1, &id_);
            id_ = other.id_;
            width_ = other.width_;
            height_ = other.height_;
            other.id_ = 0;
        }
        return *this;
    }
    
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    GLuint id() const { return id_; }
    int width() const { return width_; }
    int height() const { return height_; }
    bool valid() const { return id_ != 0; }
    
private:
    GLuint id_ = 0;
    int width_ = 0, height_ = 0;
};

// ============================================================================
// Font Management
// ============================================================================

class Font {
public:
    Font(const std::string& path, float size) : size_(size) {
        FILE* file = fopen(path.c_str(), "rb");
        if (!file) {
            // Try fallback fonts
            const char* fallbacks[] = {
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                "/usr/share/fonts/TTF/DejaVuSans.ttf",
                "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
                "/usr/share/fonts/noto/NotoSans-Regular.ttf",
                nullptr
            };
            for (int i = 0; fallbacks[i]; i++) {
                file = fopen(fallbacks[i], "rb");
                if (file) break;
            }
        }
        if (!file) return;
        
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        fontData_.resize(fileSize);
        fread(fontData_.data(), 1, fileSize, file);
        fclose(file);
        
        if (!stbtt_InitFont(&font_, fontData_.data(), 0)) {
            fontData_.clear();
            return;
        }
        
        scale_ = stbtt_ScaleForPixelHeight(&font_, size);
        stbtt_GetFontVMetrics(&font_, &ascent_, &descent_, &lineGap_);
        ascent_ = (int)(ascent_ * scale_);
        descent_ = (int)(descent_ * scale_);
        lineGap_ = (int)(lineGap_ * scale_);
        
        createAtlas();
        valid_ = true;
    }
    
    struct GlyphInfo {
        float u0, v0, u1, v1;
        float x0, y0, x1, y1;
        float advance;
        int width, height;
    };
    
    const GlyphInfo* getGlyph(int codepoint) {
        auto it = glyphs_.find(codepoint);
        if (it != glyphs_.end()) return &it->second;
        
        // Try to add glyph dynamically
        if (codepoint >= 32 && codepoint < 0x10000) {
            addGlyph(codepoint);
            it = glyphs_.find(codepoint);
            if (it != glyphs_.end()) return &it->second;
        }
        
        // Return space glyph as fallback
        it = glyphs_.find(' ');
        return it != glyphs_.end() ? &it->second : nullptr;
    }
    
    GLuint atlasTexture() const { return atlasTexture_; }
    bool valid() const { return valid_; }
    float size() const { return size_; }
    int ascent() const { return ascent_; }
    int descent() const { return descent_; }
    int lineHeight() const { return ascent_ - descent_ + lineGap_; }
    
    Size measureText(const std::string& text) {
        float width = 0;
        float maxWidth = 0;
        int lines = 1;
        
        for (size_t i = 0; i < text.size(); ) {
            if (text[i] == '\n') {
                maxWidth = std::max(maxWidth, width);
                width = 0;
                lines++;
                i++;
                continue;
            }
            
            int codepoint = decodeUTF8(text, i);
            auto* glyph = getGlyph(codepoint);
            if (glyph) {
                width += glyph->advance;
            }
        }
        
        maxWidth = std::max(maxWidth, width);
        return Size(maxWidth, lines * lineHeight());
    }
    
    static int decodeUTF8(const std::string& str, size_t& i) {
        unsigned char c = str[i];
        int codepoint;
        
        if ((c & 0x80) == 0) {
            codepoint = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            codepoint = (c & 0x1F) << 6;
            if (i + 1 < str.size()) codepoint |= (str[i+1] & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            codepoint = (c & 0x0F) << 12;
            if (i + 1 < str.size()) codepoint |= (str[i+1] & 0x3F) << 6;
            if (i + 2 < str.size()) codepoint |= (str[i+2] & 0x3F);
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            codepoint = (c & 0x07) << 18;
            if (i + 1 < str.size()) codepoint |= (str[i+1] & 0x3F) << 12;
            if (i + 2 < str.size()) codepoint |= (str[i+2] & 0x3F) << 6;
            if (i + 3 < str.size()) codepoint |= (str[i+3] & 0x3F);
            i += 4;
        } else {
            codepoint = '?';
            i += 1;
        }
        
        return codepoint;
    }
    
private:
    stbtt_fontinfo font_;
    std::vector<unsigned char> fontData_;
    float size_;
    float scale_;
    int ascent_, descent_, lineGap_;
    std::unordered_map<int, GlyphInfo> glyphs_;
    GLuint atlasTexture_ = 0;
    bool valid_ = false;
    
    static constexpr int ATLAS_WIDTH = 1024;
    static constexpr int ATLAS_HEIGHT = 1024;
    std::vector<unsigned char> atlasData_;
    int atlasX_ = 0, atlasY_ = 0, atlasRowHeight_ = 0;
    
    void createAtlas() {
        atlasData_.resize(ATLAS_WIDTH * ATLAS_HEIGHT, 0);
        atlasX_ = 1;
        atlasY_ = 1;
        atlasRowHeight_ = 0;
        
        // Pre-bake ASCII
        for (int c = 32; c < 128; ++c) {
            addGlyph(c);
        }
        
        uploadAtlas();
    }
    
    void addGlyph(int codepoint) {
        int width, height, xoff, yoff;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(
            &font_, 0, scale_, codepoint, &width, &height, &xoff, &yoff);
        
        if (!bitmap) {
            // Create empty glyph
            GlyphInfo info{};
            int advance;
            stbtt_GetCodepointHMetrics(&font_, codepoint, &advance, nullptr);
            info.advance = advance * scale_;
            glyphs_[codepoint] = info;
            return;
        }
        
        // Check if we need new row
        if (atlasX_ + width + 1 >= ATLAS_WIDTH) {
            atlasX_ = 1;
            atlasY_ += atlasRowHeight_ + 1;
            atlasRowHeight_ = 0;
        }
        
        // Check if atlas is full
        if (atlasY_ + height + 1 >= ATLAS_HEIGHT) {
            stbtt_FreeBitmap(bitmap, nullptr);
            return;
        }
        
        // Copy to atlas
        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                atlasData_[(atlasY_ + row) * ATLAS_WIDTH + (atlasX_ + col)] = 
                    bitmap[row * width + col];
            }
        }
        
        int advance;
        stbtt_GetCodepointHMetrics(&font_, codepoint, &advance, nullptr);
        
        GlyphInfo info;
        info.u0 = (float)atlasX_ / ATLAS_WIDTH;
        info.v0 = (float)atlasY_ / ATLAS_HEIGHT;
        info.u1 = (float)(atlasX_ + width) / ATLAS_WIDTH;
        info.v1 = (float)(atlasY_ + height) / ATLAS_HEIGHT;
        info.x0 = xoff;
        info.y0 = yoff;
        info.x1 = xoff + width;
        info.y1 = yoff + height;
        info.advance = advance * scale_;
        info.width = width;
        info.height = height;
        
        glyphs_[codepoint] = info;
        
        atlasX_ += width + 1;
        atlasRowHeight_ = std::max(atlasRowHeight_, height + 1);
        
        stbtt_FreeBitmap(bitmap, nullptr);
    }
    
    void uploadAtlas() {
        if (atlasTexture_) {
            glDeleteTextures(1, &atlasTexture_);
        }
        
        glGenTextures(1, &atlasTexture_);
        glBindTexture(GL_TEXTURE_2D, atlasTexture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, ATLAS_WIDTH, ATLAS_HEIGHT, 0,
                     GL_ALPHA, GL_UNSIGNED_BYTE, atlasData_.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
};

// ============================================================================
// Image Loading
// ============================================================================

class ImageLoader {
public:
    static Texture loadFromFile(const std::string& path) {
        FILE* file = fopen(path.c_str(), "rb");
        if (!file) return Texture();
        
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        std::vector<unsigned char> data(size);
        fread(data.data(), 1, size, file);
        fclose(file);
        
        return loadFromMemory(data.data(), size);
    }
    
    static Texture loadFromMemory(const unsigned char* data, size_t size) {
        int width, height, channels;
        unsigned char* pixels = stbi_load_from_memory(data, size, &width, &height, &channels, 4);
        
        if (!pixels) return Texture();
        
        Texture tex(width, height, pixels, 4);
        stbi_image_free(pixels);
        return tex;
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
        glDisable(GL_TEXTURE_2D);
        glColor4f(color.r, color.g, color.b, color.a);
        glBegin(GL_QUADS);
        glVertex2f(rect.x, rect.y);
        glVertex2f(rect.x + rect.width, rect.y);
        glVertex2f(rect.x + rect.width, rect.y + rect.height);
        glVertex2f(rect.x, rect.y + rect.height);
        glEnd();
    }
    
    void drawRoundedRect(const Rect& rect, const BorderRadius& radius, const Color& color) {
        glDisable(GL_TEXTURE_2D);
        glColor4f(color.r, color.g, color.b, color.a);
        
        float r = std::min({radius.topLeft, rect.width / 2, rect.height / 2});
        
        if (r < 1.0f) {
            drawRect(rect, color);
            return;
        }
        
        // Draw center
        drawRect(Rect(rect.x + r, rect.y, rect.width - 2*r, rect.height), color);
        
        // Draw left/right strips
        drawRect(Rect(rect.x, rect.y + r, r, rect.height - 2*r), color);
        drawRect(Rect(rect.x + rect.width - r, rect.y + r, r, rect.height - 2*r), color);
        
        // Draw corners
        drawCorner(rect.x + r, rect.y + r, r, 180, 270, color);
        drawCorner(rect.x + rect.width - r, rect.y + r, r, 270, 360, color);
        drawCorner(rect.x + rect.width - r, rect.y + rect.height - r, r, 0, 90, color);
        drawCorner(rect.x + r, rect.y + rect.height - r, r, 90, 180, color);
    }
    
    void drawBorder(const Rect& rect, const BorderRadius& radius, 
                   const Color& color, float width) {
        glDisable(GL_TEXTURE_2D);
        glLineWidth(width);
        glColor4f(color.r, color.g, color.b, color.a);
        
        float r = std::min({radius.topLeft, rect.width / 2, rect.height / 2});
        
        if (r < 1.0f) {
            glBegin(GL_LINE_LOOP);
            glVertex2f(rect.x, rect.y);
            glVertex2f(rect.x + rect.width, rect.y);
            glVertex2f(rect.x + rect.width, rect.y + rect.height);
            glVertex2f(rect.x, rect.y + rect.height);
            glEnd();
            return;
        }
        
        glBegin(GL_LINE_STRIP);
        // Top edge
        glVertex2f(rect.x + r, rect.y);
        glVertex2f(rect.x + rect.width - r, rect.y);
        
        // Top-right corner
        drawCornerArc(rect.x + rect.width - r, rect.y + r, r, 270, 360);
        
        // Right edge
        glVertex2f(rect.x + rect.width, rect.y + r);
        glVertex2f(rect.x + rect.width, rect.y + rect.height - r);
        
        // Bottom-right corner
        drawCornerArc(rect.x + rect.width - r, rect.y + rect.height - r, r, 0, 90);
        
        // Bottom edge
        glVertex2f(rect.x + rect.width - r, rect.y + rect.height);
        glVertex2f(rect.x + r, rect.y + rect.height);
        
        // Bottom-left corner
        drawCornerArc(rect.x + r, rect.y + rect.height - r, r, 90, 180);
        
        // Left edge
        glVertex2f(rect.x, rect.y + rect.height - r);
        glVertex2f(rect.x, rect.y + r);
        
        // Top-left corner
        drawCornerArc(rect.x + r, rect.y + r, r, 180, 270);
        
        glVertex2f(rect.x + r, rect.y);
        glEnd();
    }
    
    void drawGradient(const Rect& rect, const Color& start, const Color& end, float angle) {
        glDisable(GL_TEXTURE_2D);
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
                 const Color& color, const TextStyle& style = TextStyle()) {
        if (!font || !font->valid()) return;
        
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, font->atlasTexture());
        glColor4f(color.r, color.g, color.b, color.a);
        
        float x = pos.x;
        float y = pos.y + font->ascent();
        float startX = x;
        
        glBegin(GL_QUADS);
        for (size_t i = 0; i < text.size(); ) {
            if (text[i] == '\n') {
                x = startX;
                y += font->lineHeight();
                i++;
                continue;
            }
            
            int codepoint = Font::decodeUTF8(text, i);
            auto* glyph = font->getGlyph(codepoint);
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
    
    void drawImage(const Texture& texture, const Rect& rect, float opacity = 1.0f) {
        if (!texture.valid()) return;
        
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture.id());
        glColor4f(1.0f, 1.0f, 1.0f, opacity);
        
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(rect.x, rect.y);
        glTexCoord2f(1, 0); glVertex2f(rect.x + rect.width, rect.y);
        glTexCoord2f(1, 1); glVertex2f(rect.x + rect.width, rect.y + rect.height);
        glTexCoord2f(0, 1); glVertex2f(rect.x, rect.y + rect.height);
        glEnd();
        
        glDisable(GL_TEXTURE_2D);
    }
    
    void drawImageScaled(const Texture& texture, const Rect& rect, 
                        bool preserveAspect = true, float opacity = 1.0f) {
        if (!texture.valid()) return;
        
        Rect destRect = rect;
        
        if (preserveAspect) {
            float imgAspect = (float)texture.width() / texture.height();
            float rectAspect = rect.width / rect.height;
            
            if (imgAspect > rectAspect) {
                // Image wider than rect
                float newHeight = rect.width / imgAspect;
                destRect.y += (rect.height - newHeight) / 2;
                destRect.height = newHeight;
            } else {
                // Image taller than rect
                float newWidth = rect.height * imgAspect;
                destRect.x += (rect.width - newWidth) / 2;
                destRect.width = newWidth;
            }
        }
        
        drawImage(texture, destRect, opacity);
    }
    
    Font* loadFont(const std::string& path, float size) {
        std::string key = path + ":" + std::to_string((int)size);
        auto it = fonts_.find(key);
        if (it != fonts_.end()) {
            return it->second.get();
        }
        
        auto font = std::make_unique<Font>(path, size);
        if (!font->valid()) {
            // Try loading default font
            font = std::make_unique<Font>("", size);
        }
        
        Font* ptr = font.get();
        fonts_[key] = std::move(font);
        return ptr;
    }
    
    Texture* loadImage(const std::string& path) {
        auto it = textures_.find(path);
        if (it != textures_.end()) {
            return it->second.get();
        }
        
        auto tex = std::make_unique<Texture>(ImageLoader::loadFromFile(path));
        if (!tex->valid()) return nullptr;
        
        Texture* ptr = tex.get();
        textures_[path] = std::move(tex);
        return ptr;
    }
    
    void unloadImage(const std::string& path) {
        textures_.erase(path);
    }
    
private:
    int width_, height_;
    std::unordered_map<std::string, std::unique_ptr<Font>> fonts_;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures_;
    
    void drawCorner(float cx, float cy, float radius, float startAngle, float endAngle, 
                   const Color& color) {
        const int segments = 8;
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int i = 0; i <= segments; ++i) {
            float angle = startAngle + (endAngle - startAngle) * i / segments;
            float rad = angle * 3.14159f / 180.0f;
            glVertex2f(cx + cos(rad) * radius, cy + sin(rad) * radius);
        }
        glEnd();
    }
    
    void drawCornerArc(float cx, float cy, float radius, float startAngle, float endAngle) {
        const int segments = 8;
        for (int i = 0; i <= segments; ++i) {
            float angle = startAngle + (endAngle - startAngle) * i / segments;
            float rad = angle * 3.14159f / 180.0f;
            glVertex2f(cx + cos(rad) * radius, cy + sin(rad) * radius);
        }
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
