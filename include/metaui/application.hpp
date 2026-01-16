#pragma once

#include "core.hpp"
#include "widget.hpp"
#include "renderer.hpp"
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <chrono>
#include <linux/input-event-codes.h>  // For BTN_LEFT, etc.
// Include wlr-layer-shell protocol (for shell apps)
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

namespace MetaUI {

// ============================================================================
// Wayland Application
// ============================================================================

class Application {
public:
    Application(const std::string& title, int width, int height)
        : title_(title), width_(width), height_(height) {
        initWayland();
        initEGL();
        renderer_ = std::make_unique<Renderer>(width, height);
    }
    
    ~Application() {
        cleanup();
    }
    
    // Set root widget
    void setRoot(WidgetPtr root) {
        root_ = std::move(root);
        if (root_) {
            root_->measure(Size(width_, height_));
            root_->layout(Rect(0, 0, width_, height_));
        }
    }
    
    // Run the event loop
    void run() {
        running_ = true;
        
        auto lastFrame = std::chrono::steady_clock::now();
        
        while (running_ && wl_display_dispatch(display_) != -1) {
            auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(now - lastFrame).count();
            lastFrame = now;
            
            // Update animations
            update(dt);
            
            // Render
            render();
            
            eglSwapBuffers(eglDisplay_, eglSurface_);
        }
    }
    
    void quit() {
        running_ = false;
    }
    
    Renderer& renderer() { return *renderer_; }
    
private:
    std::string title_;
    int width_, height_;
    bool running_ = false;
    
    // Wayland
    wl_display* display_ = nullptr;
    wl_registry* registry_ = nullptr;
    wl_compositor* compositor_ = nullptr;
    wl_surface* surface_ = nullptr;
    wl_seat* seat_ = nullptr;
    wl_pointer* pointer_ = nullptr;
    wl_keyboard* keyboard_ = nullptr;
    zwlr_layer_shell_v1* layerShell_ = nullptr;
    zwlr_layer_surface_v1* layerSurface_ = nullptr;
    
    // EGL
    EGLDisplay eglDisplay_ = EGL_NO_DISPLAY;
    EGLContext eglContext_ = EGL_NO_CONTEXT;
    EGLSurface eglSurface_ = EGL_NO_SURFACE;
    wl_egl_window* eglWindow_ = nullptr;
    
    // UI
    WidgetPtr root_;
    WidgetPtr focusedWidget_;
    std::unique_ptr<Renderer> renderer_;
    
    void initWayland() {
        display_ = wl_display_connect(nullptr);
        if (!display_) {
            throw std::runtime_error("Failed to connect to Wayland display");
        }
        
        registry_ = wl_display_get_registry(display_);
        
        static const wl_registry_listener registryListener = {
            .global = [](void* data, wl_registry* registry, uint32_t name,
                        const char* interface, uint32_t version) {
                auto* app = static_cast<Application*>(data);
                
                if (strcmp(interface, wl_compositor_interface.name) == 0) {
                    app->compositor_ = static_cast<wl_compositor*>(
                        wl_registry_bind(registry, name, &wl_compositor_interface, 4));
                } else if (strcmp(interface, wl_seat_interface.name) == 0) {
                    app->seat_ = static_cast<wl_seat*>(
                        wl_registry_bind(registry, name, &wl_seat_interface, 5));
                    app->setupSeat();
                } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
                    app->layerShell_ = static_cast<zwlr_layer_shell_v1*>(
                        wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1));
                }
            },
            .global_remove = [](void*, wl_registry*, uint32_t) {}
        };
        
        wl_registry_add_listener(registry_, &registryListener, this);
        wl_display_roundtrip(display_);
        
        if (!compositor_) {
            throw std::runtime_error("Wayland compositor not available");
        }
        
        surface_ = wl_compositor_create_surface(compositor_);
        
        // Create layer surface for shell apps
        if (layerShell_) {
            layerSurface_ = zwlr_layer_shell_v1_get_layer_surface(
                layerShell_, surface_, nullptr,
                ZWLR_LAYER_SHELL_V1_LAYER_TOP, "metaui");
            
            zwlr_layer_surface_v1_set_size(layerSurface_, width_, height_);
            zwlr_layer_surface_v1_set_anchor(layerSurface_,
                ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
            
            static const zwlr_layer_surface_v1_listener layerListener = {
                .configure = [](void* data, zwlr_layer_surface_v1* surface,
                               uint32_t serial, uint32_t w, uint32_t h) {
                    zwlr_layer_surface_v1_ack_configure(surface, serial);
                },
                .closed = [](void* data, zwlr_layer_surface_v1* surface) {
                    static_cast<Application*>(data)->quit();
                }
            };
            
            zwlr_layer_surface_v1_add_listener(layerSurface_, &layerListener, this);
            wl_surface_commit(surface_);
            wl_display_roundtrip(display_);
        }
    }
    
    void setupSeat() {
        static const wl_seat_listener seatListener = {
            .capabilities = [](void* data, wl_seat* seat, uint32_t caps) {
                auto* app = static_cast<Application*>(data);
                
                if (caps & WL_SEAT_CAPABILITY_POINTER) {
                    app->pointer_ = wl_seat_get_pointer(seat);
                    app->setupPointer();
                }
                
                if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
                    app->keyboard_ = wl_seat_get_keyboard(seat);
                    app->setupKeyboard();
                }
            },
            .name = [](void*, wl_seat*, const char*) {}
        };
        
        wl_seat_add_listener(seat_, &seatListener, this);
    }
    
    void setupPointer() {
        static const wl_pointer_listener pointerListener = {
            .enter = [](void*, wl_pointer*, uint32_t, wl_surface*, 
                       wl_fixed_t, wl_fixed_t) {},
            .leave = [](void*, wl_pointer*, uint32_t, wl_surface*) {},
            .motion = [](void* data, wl_pointer*, uint32_t time,
                        wl_fixed_t x, wl_fixed_t y) {
                auto* app = static_cast<Application*>(data);
                MouseEvent event;
                event.position = Point(wl_fixed_to_double(x), wl_fixed_to_double(y));
                if (app->root_) {
                    app->root_->handleMouseMove(event);
                }
            },
            .button = [](void* data, wl_pointer*, uint32_t, uint32_t time,
                        uint32_t button, uint32_t state) {
                auto* app = static_cast<Application*>(data);
                MouseEvent event;
                event.button = static_cast<MouseButton>(button - BTN_LEFT);
                event.pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
                if (app->root_) {
                    app->root_->handleMouseButton(event);
                }
            },
            .axis = [](void* data, wl_pointer*, uint32_t, uint32_t axis,
                      wl_fixed_t value) {
                auto* app = static_cast<Application*>(data);
                ScrollEvent event;
                if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
                    event.deltaY = wl_fixed_to_double(value);
                } else {
                    event.deltaX = wl_fixed_to_double(value);
                }
                if (app->root_) {
                    app->root_->handleScroll(event);
                }
            },
            .frame = [](void*, wl_pointer*) {},
            .axis_source = [](void*, wl_pointer*, uint32_t) {},
            .axis_stop = [](void*, wl_pointer*, uint32_t, uint32_t) {},
            .axis_discrete = [](void*, wl_pointer*, uint32_t, int32_t) {},
        };
        
        wl_pointer_add_listener(pointer_, &pointerListener, this);
    }
    
    void setupKeyboard() {
        static const wl_keyboard_listener keyboardListener = {
            .keymap = [](void*, wl_keyboard*, uint32_t, int32_t, uint32_t) {},
            .enter = [](void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*) {},
            .leave = [](void*, wl_keyboard*, uint32_t, wl_surface*) {},
            .key = [](void* data, wl_keyboard*, uint32_t, uint32_t time,
                     uint32_t key, uint32_t state) {
                auto* app = static_cast<Application*>(data);
                KeyEvent event;
                event.keycode = key;
                event.pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);
                if (app->root_) {
                    app->root_->handleKeyEvent(event);
                }
            },
            .modifiers = [](void*, wl_keyboard*, uint32_t, uint32_t, uint32_t,
                           uint32_t, uint32_t) {},
            .repeat_info = [](void*, wl_keyboard*, int32_t, int32_t) {}
        };
        
        wl_keyboard_add_listener(keyboard_, &keyboardListener, this);
    }
    
    void initEGL() {
        eglDisplay_ = eglGetDisplay((EGLNativeDisplayType)display_);
        if (eglDisplay_ == EGL_NO_DISPLAY) {
            throw std::runtime_error("Failed to get EGL display");
        }
        
        if (!eglInitialize(eglDisplay_, nullptr, nullptr)) {
            throw std::runtime_error("Failed to initialize EGL");
        }
        
        EGLint configAttribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_NONE
        };
        
        EGLConfig config;
        EGLint numConfigs;
        if (!eglChooseConfig(eglDisplay_, configAttribs, &config, 1, &numConfigs)) {
            throw std::runtime_error("Failed to choose EGL config");
        }
        
        eglBindAPI(EGL_OPENGL_API);
        
        EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        
        eglContext_ = eglCreateContext(eglDisplay_, config, EGL_NO_CONTEXT, contextAttribs);
        if (eglContext_ == EGL_NO_CONTEXT) {
            throw std::runtime_error("Failed to create EGL context");
        }
        
        eglWindow_ = wl_egl_window_create(surface_, width_, height_);
        eglSurface_ = eglCreateWindowSurface(eglDisplay_, config, 
                                            (EGLNativeWindowType)eglWindow_, nullptr);
        
        eglMakeCurrent(eglDisplay_, eglSurface_, eglSurface_, eglContext_);
    }
    
    void update(float dt) {
        // Update logic here (animations, etc.)
    }
    
    void render() {
        renderer_->beginFrame();
        
        if (root_) {
            root_->render(*renderer_);
        }
        
        renderer_->endFrame();
    }
    
    void cleanup() {
        if (eglDisplay_ != EGL_NO_DISPLAY) {
            eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (eglContext_ != EGL_NO_CONTEXT) {
                eglDestroyContext(eglDisplay_, eglContext_);
            }
            if (eglSurface_ != EGL_NO_SURFACE) {
                eglDestroySurface(eglDisplay_, eglSurface_);
            }
            eglTerminate(eglDisplay_);
        }
        
        if (eglWindow_) wl_egl_window_destroy(eglWindow_);
        if (layerSurface_) zwlr_layer_surface_v1_destroy(layerSurface_);
        if (surface_) wl_surface_destroy(surface_);
        if (pointer_) wl_pointer_destroy(pointer_);
        if (keyboard_) wl_keyboard_destroy(keyboard_);
        if (seat_) wl_seat_destroy(seat_);
        if (layerShell_) zwlr_layer_shell_v1_destroy(layerShell_);
        if (compositor_) wl_compositor_destroy(compositor_);
        if (registry_) wl_registry_destroy(registry_);
        if (display_) wl_display_disconnect(display_);
    }
};

} // namespace MetaUI
