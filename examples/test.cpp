#include <metaui.hpp>
#include <iostream>

using namespace MetaUI;

int main() {
    std::cout << "==================================" << std::endl;
    std::cout << "  MetaUI Framework - Simple Test" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;
    
    try {
        std::cout << "[1/4] Creating Wayland application..." << std::endl;
        Application app("MetaUI Simple Test", 500, 400);
        std::cout << "      ✓ Application created" << std::endl;
        
        std::cout << "[2/4] Building UI layout..." << std::endl;
        
        // Get dark theme
        auto theme = Theme::dark();
        
        // Create root vertical layout
        auto root = std::make_shared<Box>(Direction::Vertical);
   root->padding(30);
root->spacing(15);
root->background(theme.background);
        
        // Title
        auto title = std::make_shared<Text>("Welcome to MetaUI!");
        title->fontSize(28)
              .color(theme.text)
              .bold(true);
        root->addChild(title);
        
        // Subtitle
        auto subtitle = std::make_shared<Text>("A modern C++ GUI framework for Wayland");
        subtitle->fontSize(14)
                .color(theme.textMuted);
        root->addChild(subtitle);
        
        // Divider
        root->addChild(std::make_shared<Divider>());
        root->addChild(std::make_shared<Spacer>(10));
        
        // Info text
        auto info = std::make_shared<Text>("This is a basic test of the MetaUI framework.");
        info->fontSize(12)
            .color(theme.text);
        root->addChild(info);
        
        // Button row
        auto buttonRow = std::make_shared<Box>(Direction::Horizontal);
        buttonRow->spacing(10).align(Alignment::Center);
        
        auto button1 = std::make_shared<Button>("Click Me");
        button1->background(theme.primary)
               .onClick([]() {
                   std::cout << "      → Button 1 clicked!" << std::endl;
               });
        
        auto button2 = std::make_shared<Button>("Exit");
        button2->background(theme.error)
               .onClick([&app]() {
                   std::cout << "      → Exit button clicked, quitting..." << std::endl;
                   app.quit();
               });
        
        buttonRow->addChild(button1);
        buttonRow->addChild(button2);
        root->addChild(buttonRow);
        
        // Spacer
        root->addChild(std::make_shared<Spacer>(10));
        
        // Checkbox
        auto checkRow = std::make_shared<Box>(Direction::Horizontal);
        checkRow->spacing(10);
        
        auto checkbox = std::make_shared<Checkbox>(false);
        checkbox->onToggle([](bool checked) {
            std::cout << "      → Checkbox: " << (checked ? "ON" : "OFF") << std::endl;
        });
        
        auto checkLabel = std::make_shared<Text>("Enable feature");
        checkLabel->fontSize(12).color(theme.text);
        
        checkRow->addChild(checkbox);
        checkRow->addChild(checkLabel);
        root->addChild(checkRow);
        
        // Slider
        root->addChild(std::make_shared<Spacer>(5));
        auto sliderLabel = std::make_shared<Text>("Volume:");
        sliderLabel->fontSize(12).color(theme.textMuted);
        root->addChild(sliderLabel);
        
        auto slider = std::make_shared<Slider>(0.0f, 100.0f, 50.0f);
        slider->fillColor(theme.primary)
              .onChange([](float value) {
                  std::cout << "      → Slider: " << (int)value << "%" << std::endl;
              });
        root->addChild(slider);
        
        // Progress bar
        root->addChild(std::make_shared<Spacer>(5));
        auto progressLabel = std::make_shared<Text>("Progress:");
        progressLabel->fontSize(12).color(theme.textMuted);
        root->addChild(progressLabel);
        
        auto progress = std::make_shared<ProgressBar>(0.65f);
        progress->fillColor(theme.success);
        root->addChild(progress);
        
        std::cout << "      ✓ UI layout created" << std::endl;
        
        std::cout << "[3/4] Setting root widget..." << std::endl;
        app.setRoot(root);
        std::cout << "      ✓ Root widget set" << std::endl;
        
        std::cout << "[4/4] Starting main loop..." << std::endl;
        std::cout << std::endl;
        std::cout << "Application is running!" << std::endl;
        std::cout << "- Click buttons to see interactions" << std::endl;
        std::cout << "- Click 'Exit' button or press Ctrl+C to quit" << std::endl;
        std::cout << std::endl;
        
        app.run();
        
        std::cout << std::endl;
        std::cout << "Application exited normally." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << std::endl;
        std::cerr << "ERROR: " << e.what() << std::endl;
        std::cerr << std::endl;
        std::cerr << "Common issues:" << std::endl;
        std::cerr << "1. Not running on Wayland (this framework requires Wayland)" << std::endl;
        std::cerr << "2. Missing widget implementations (see FIXES.md)" << std::endl;
        std::cerr << "3. Compositor doesn't support wlr-layer-shell" << std::endl;
        std::cerr << std::endl;
        return 1;
    }
    
    return 0;
}
