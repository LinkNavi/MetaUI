#pragma once

/*
 * MetaUI - Modern C++ GUI Framework for Wayland
 * 
 * A header-only, easy-to-use GUI framework optimized for shell applications
 * on wlroots-based compositors like StarView.
 * 
 * Features:
 * - Header-only, modular design
 * - OpenGL-accelerated rendering
 * - Flexible layout system (Box, Stack, Grid, Sidebar)
 * - Rich widget library (Text, Button, Slider, TextInput, etc.)
 * - Easy styling with gradients, shadows, rounded corners
 * - Animation support with multiple easing curves
 * - Native Wayland integration with wlr-layer-shell
 * 
 * Dependencies:
 * - OpenGL, EGL
 * - Wayland client libraries
 * - wlr-layer-shell protocol
 * - stb_truetype.h (included)
 * 
 * Usage:
 * 
 *   #include <metaui.hpp>
 *   using namespace MetaUI;
 *   
 *   int main() {
 *       Application app("My App", 400, 600);
 *       
 *       auto root = Box(Direction::Vertical)
 *           .spacing(10)
 *           .padding(20);
 *       
 *       root.addChild(
 *           Text("Hello, MetaUI!")
 *               .fontSize(24)
 *               .color(Color::fromHex(0xffffffff))
 *       );
 *       
 *       app.setRoot(std::make_shared<decltype(root)>(root));
 *       app.run();
 *   }
 * 
 * License: MIT
 */

#include "metaui/core.hpp"
#include "metaui/widget.hpp"
#include "metaui/layouts.hpp"
#include "metaui/renderer.hpp"  // MUST come before widgets.hpp
#include "metaui/widgets.hpp"
#include "metaui/application.hpp"

// Convenience namespace
namespace MetaUI {

// ============================================================================
// Builder Helpers
// ============================================================================

// Helper to create shared_ptr from widget
template<typename T, typename... Args>
std::shared_ptr<T> make(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// ============================================================================
// Common Color Palette
// ============================================================================

namespace Colors {
    // Basic colors
    inline Color black() { return Color(0, 0, 0, 1); }
    inline Color white() { return Color(1, 1, 1, 1); }
    inline Color transparent() { return Color(0, 0, 0, 0); }
    
    // Grays
    inline Color gray(float v) { return Color(v, v, v, 1); }
    inline Color gray50() { return Color(0.98f, 0.98f, 0.98f, 1); }
    inline Color gray100() { return Color(0.96f, 0.96f, 0.97f, 1); }
    inline Color gray200() { return Color(0.89f, 0.90f, 0.93f, 1); }
    inline Color gray300() { return Color(0.82f, 0.84f, 0.87f, 1); }
    inline Color gray400() { return Color(0.63f, 0.67f, 0.72f, 1); }
    inline Color gray500() { return Color(0.46f, 0.51f, 0.58f, 1); }
    inline Color gray600() { return Color(0.35f, 0.40f, 0.48f, 1); }
    inline Color gray700() { return Color(0.26f, 0.31f, 0.38f, 1); }
    inline Color gray800() { return Color(0.18f, 0.22f, 0.28f, 1); }
    inline Color gray900() { return Color(0.11f, 0.13f, 0.18f, 1); }
    
    // Catppuccin Mocha (matches StarView config)
    inline Color ctp_rosewater() { return Color::fromHex(0xf5e0dcff); }
    inline Color ctp_flamingo() { return Color::fromHex(0xf2cdcdff); }
    inline Color ctp_pink() { return Color::fromHex(0xf5c2e7ff); }
    inline Color ctp_mauve() { return Color::fromHex(0xcba6f7ff); }
    inline Color ctp_red() { return Color::fromHex(0xf38ba8ff); }
    inline Color ctp_maroon() { return Color::fromHex(0xeba0acff); }
    inline Color ctp_peach() { return Color::fromHex(0xfab387ff); }
    inline Color ctp_yellow() { return Color::fromHex(0xf9e2afff); }
    inline Color ctp_green() { return Color::fromHex(0xa6e3a1ff); }
    inline Color ctp_teal() { return Color::fromHex(0x94e2d5ff); }
    inline Color ctp_sky() { return Color::fromHex(0x89dcebff); }
    inline Color ctp_sapphire() { return Color::fromHex(0x74c7ecff); }
    inline Color ctp_blue() { return Color::fromHex(0x89b4faff); }
    inline Color ctp_lavender() { return Color::fromHex(0xb4befeff); }
    inline Color ctp_text() { return Color::fromHex(0xcdd6f4ff); }
    inline Color ctp_subtext1() { return Color::fromHex(0xbac2deff); }
    inline Color ctp_subtext0() { return Color::fromHex(0xa6adc8ff); }
    inline Color ctp_overlay2() { return Color::fromHex(0x9399b2ff); }
    inline Color ctp_overlay1() { return Color::fromHex(0x7f849cff); }
    inline Color ctp_overlay0() { return Color::fromHex(0x6c7086ff); }
    inline Color ctp_surface2() { return Color::fromHex(0x585b70ff); }
    inline Color ctp_surface1() { return Color::fromHex(0x45475aff); }
    inline Color ctp_surface0() { return Color::fromHex(0x313244ff); }
    inline Color ctp_base() { return Color::fromHex(0x1e1e2eff); }
    inline Color ctp_mantle() { return Color::fromHex(0x181825ff); }
    inline Color ctp_crust() { return Color::fromHex(0x11111bff); }
}

// ============================================================================
// Pre-configured Themes
// ============================================================================

struct Theme {
    Color background;
    Color surface;
    Color primary;
    Color secondary;
    Color text;
    Color textMuted;
    Color border;
    Color success;
    Color warning;
    Color error;
    
    static Theme dark() {
        return Theme{
            .background = Colors::ctp_base(),
            .surface = Colors::ctp_surface0(),
            .primary = Colors::ctp_blue(),
            .secondary = Colors::ctp_mauve(),
            .text = Colors::ctp_text(),
            .textMuted = Colors::ctp_overlay0(),
            .border = Colors::ctp_surface1(),
            .success = Colors::ctp_green(),
            .warning = Colors::ctp_yellow(),
            .error = Colors::ctp_red()
        };
    }
    
    static Theme light() {
        return Theme{
            .background = Colors::white(),
            .surface = Colors::gray50(),
            .primary = Color::fromHex(0x3b82f6ff),
            .secondary = Color::fromHex(0x8b5cf6ff),
            .text = Colors::gray900(),
            .textMuted = Colors::gray500(),
            .border = Colors::gray200(),
            .success = Color::fromHex(0x10b981ff),
            .warning = Color::fromHex(0xf59e0bff),
            .error = Color::fromHex(0xef4444ff)
        };
    }
};

} // namespace MetaUI



