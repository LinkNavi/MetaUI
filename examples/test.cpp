#include <metaui.hpp>
#include <iostream>

using namespace MetaUI;

int main() {
    std::cout << "MetaUI Text & Image Test\n";
    std::cout << "========================\n\n";
    
    try {
        Application app("MetaUI Test", 600, 500);
        auto theme = Theme::dark();
        
        // Root layout
        auto root = std::make_shared<Box>(Direction::Vertical);
        root->padding(30).spacing(20).background(theme.background);
        
        // Title
        auto title = std::make_shared<Text>("MetaUI Framework");
        title->fontSize(32).color(theme.text).bold(true);
        root->addChild(title);
        
        // Subtitle with UTF-8
        auto subtitle = std::make_shared<Text>("Modern C++ GUI • Wayland • UTF-8: äöü 中文 🎉");
        subtitle->fontSize(14).color(theme.textMuted);
        root->addChild(subtitle);
        
        root->addChild(std::make_shared<Divider>());
        
        // Image section
        auto imgSection = std::make_shared<Box>(Direction::Horizontal);
        imgSection->spacing(20).align(Alignment::Center);
        
        // Placeholder image (will show gray box if file doesn't exist)
        auto img = std::make_shared<Image>("/tmp/test.bmp");
        img->width(SizeSpec::fixed(150)).height(SizeSpec::fixed(100));
        img->preserveAspect(true);
        imgSection->addChild(img);
        
        auto imgDesc = std::make_shared<Box>(Direction::Vertical);
        imgDesc->spacing(8);
        
        auto imgTitle = std::make_shared<Text>("Image Widget");
        imgTitle->fontSize(18).color(theme.text).bold(true);
        imgDesc->addChild(imgTitle);
        
        auto imgText = std::make_shared<Text>("Supports BMP images.\nLoad from file or memory.");
        imgText->fontSize(12).color(theme.textMuted);
        imgDesc->addChild(imgText);
        
        imgSection->addChild(imgDesc);
        root->addChild(imgSection);
        
        root->addChild(std::make_shared<Spacer>(10));
        
        // Text examples
        auto textSection = std::make_shared<Box>(Direction::Vertical);
        textSection->spacing(10);
        
        auto textTitle = std::make_shared<Text>("Text Rendering Features:");
        textTitle->fontSize(16).color(theme.primary);
        textSection->addChild(textTitle);
        
        auto sizes = std::make_shared<Box>(Direction::Horizontal);
        sizes->spacing(15);
        
        auto small = std::make_shared<Text>("Small (10)");
        small->fontSize(10).color(theme.text);
        sizes->addChild(small);
        
        auto medium = std::make_shared<Text>("Medium (14)");
        medium->fontSize(14).color(theme.text);
        sizes->addChild(medium);
        
        auto large = std::make_shared<Text>("Large (20)");
        large->fontSize(20).color(theme.text);
        sizes->addChild(large);
        
        textSection->addChild(sizes);
        
        // Colored text
        auto colors = std::make_shared<Box>(Direction::Horizontal);
        colors->spacing(10);
        
        auto red = std::make_shared<Text>("Error");
        red->fontSize(14).color(theme.error);
        colors->addChild(red);
        
        auto green = std::make_shared<Text>("Success");
        green->fontSize(14).color(theme.success);
        colors->addChild(green);
        
        auto blue = std::make_shared<Text>("Primary");
        blue->fontSize(14).color(theme.primary);
        colors->addChild(blue);
        
        textSection->addChild(colors);
        root->addChild(textSection);
        
        root->addChild(std::make_shared<Spacer>(10));
        
        // Button row
        auto buttonRow = std::make_shared<Box>(Direction::Horizontal);
        buttonRow->spacing(10).align(Alignment::Center);
        
        auto btn1 = std::make_shared<Button>("Click Me");
        btn1->background(theme.primary).onClick([]() {
            std::cout << "Button clicked!\n";
        });
        buttonRow->addChild(btn1);
        
        auto btn2 = std::make_shared<Button>("Exit");
        btn2->background(theme.error).onClick([&app]() {
            app.quit();
        });
        buttonRow->addChild(btn2);
        
        root->addChild(buttonRow);
        
        // Progress bar
        auto progress = std::make_shared<ProgressBar>(0.7f);
        progress->fillColor(theme.success);
        root->addChild(progress);
        
        app.setRoot(root);
        
        std::cout << "Running... Press Exit button or Ctrl+C to quit.\n\n";
        app.run();
        
        std::cout << "\nExited normally.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        std::cerr << "\nMake sure you're running on Wayland with wlr-layer-shell support.\n";
        return 1;
    }
    
    return 0;
}
