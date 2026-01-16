#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   MetaUI Framework Installer          ║${NC}"
echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo ""

# Check for required tools
echo -e "${YELLOW}Checking dependencies...${NC}"

MISSING_DEPS=()

if ! command -v git &> /dev/null; then
    MISSING_DEPS+=("git")
fi

if ! command -v meson &> /dev/null; then
    MISSING_DEPS+=("meson")
fi

if ! command -v ninja &> /dev/null; then
    MISSING_DEPS+=("ninja")
fi

if ! command -v pkg-config &> /dev/null; then
    MISSING_DEPS+=("pkg-config")
fi

if ! command -v wayland-scanner &> /dev/null; then
    MISSING_DEPS+=("wayland-scanner")
fi

# Check for development libraries
if ! pkg-config --exists wayland-client 2>/dev/null; then
    MISSING_DEPS+=("wayland-client-dev")
fi

if ! pkg-config --exists wayland-egl 2>/dev/null; then
    MISSING_DEPS+=("wayland-egl-dev")
fi

if ! pkg-config --exists egl 2>/dev/null; then
    MISSING_DEPS+=("libegl-dev")
fi

if ! pkg-config --exists gl 2>/dev/null; then
    MISSING_DEPS+=("libgl-dev")
fi

if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
    echo -e "${RED}Missing dependencies:${NC}"
    for dep in "${MISSING_DEPS[@]}"; do
        echo -e "  - ${dep}"
    done
    echo ""
    echo -e "${YELLOW}To install on Ubuntu/Debian:${NC}"
    echo "  sudo apt update"
    echo "  sudo apt install git meson ninja-build pkg-config wayland-protocols libwayland-dev libwayland-egl1 libegl1-mesa-dev libgl1-mesa-dev wget"
    echo ""
    echo -e "${YELLOW}To install on Arch Linux:${NC}"
    echo "  sudo pacman -S git meson ninja pkg-config wayland wayland-protocols mesa wget"
    echo ""
    echo -e "${YELLOW}To install on Fedora:${NC}"
    echo "  sudo dnf install git meson ninja-build pkg-config wayland-devel wayland-protocols-devel mesa-libEGL-devel mesa-libGL-devel wget"
    exit 1
fi

echo -e "${GREEN}✓ All dependencies found${NC}"
echo ""

# Clone or update repository
REPO_URL="https://github.com/LinkNavi/MetaUI/"
INSTALL_DIR="${HOME}/.local/src/metaui"

if [ -d "$INSTALL_DIR" ]; then
    echo -e "${YELLOW}Updating existing installation...${NC}"
    cd "$INSTALL_DIR"
    git pull
else
    echo -e "${YELLOW}Cloning MetaUI repository...${NC}"
    mkdir -p "$(dirname "$INSTALL_DIR")"
    git clone "$REPO_URL" "$INSTALL_DIR"
    cd "$INSTALL_DIR"
fi

echo -e "${GREEN}✓ Repository ready${NC}"
echo ""

# Download stb_truetype.h if not present
if [ ! -f "include/metaui/stb_truetype.h" ]; then
    echo -e "${YELLOW}Downloading stb_truetype.h...${NC}"
    mkdir -p include/metaui
    wget https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h \
         -O include/metaui/stb_truetype.h
    echo -e "${GREEN}✓ stb_truetype.h downloaded${NC}"
    echo ""
fi

# Download wlr-layer-shell protocol
echo -e "${YELLOW}Downloading wlr-layer-shell protocol...${NC}"
mkdir -p protocols
if [ ! -f "protocols/wlr-layer-shell-unstable-v1.xml" ]; then
    wget https://gitlab.freedesktop.org/wlroots/wlr-protocols/-/raw/master/unstable/wlr-layer-shell-unstable-v1.xml \
         -O protocols/wlr-layer-shell-unstable-v1.xml
fi
echo -e "${GREEN}✓ Protocol downloaded${NC}"
echo ""

# Build with meson
echo -e "${YELLOW}Configuring build...${NC}"
if [ -d "build" ]; then
    rm -rf build
fi
meson setup build --prefix="${HOME}/.local"

echo -e "${GREEN}✓ Build configured${NC}"
echo ""

echo -e "${YELLOW}Building MetaUI...${NC}"
meson compile -C build

echo -e "${GREEN}✓ Build complete${NC}"
echo ""

# Install
echo -e "${YELLOW}Installing MetaUI...${NC}"
meson install -C build

echo -e "${GREEN}✓ Installation complete${NC}"
echo ""

# Update environment
if ! grep -q "${HOME}/.local/bin" <<< "$PATH"; then
    echo -e "${YELLOW}Adding ${HOME}/.local/bin to PATH...${NC}"
    
    SHELL_RC=""
    if [ -n "$BASH_VERSION" ]; then
        SHELL_RC="${HOME}/.bashrc"
    elif [ -n "$ZSH_VERSION" ]; then
        SHELL_RC="${HOME}/.zshrc"
    fi
    
    if [ -n "$SHELL_RC" ]; then
        echo "" >> "$SHELL_RC"
        echo "# Added by MetaUI installer" >> "$SHELL_RC"
        echo 'export PATH="${HOME}/.local/bin:${PATH}"' >> "$SHELL_RC"
        echo 'export PKG_CONFIG_PATH="${HOME}/.local/lib/pkgconfig:${PKG_CONFIG_PATH}"' >> "$SHELL_RC"
        echo -e "${GREEN}✓ PATH updated in ${SHELL_RC}${NC}"
        echo -e "${YELLOW}  Please run: source ${SHELL_RC}${NC}"
    fi
fi

echo ""
echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   Installation Complete!              ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""
echo -e "${GREEN}MetaUI has been installed to: ${HOME}/.local${NC}"
echo ""
echo -e "${YELLOW}To test the installation, run:${NC}"
echo -e "  ${HOME}/.local/bin/metaui-test"
echo ""
echo -e "${YELLOW}To use MetaUI in your projects:${NC}"
echo -e "  #include <metaui.hpp>"
echo ""
echo -e "${YELLOW}Compile with:${NC}"
echo -e '  g++ -o myapp myapp.cpp $(pkg-config --cflags --libs metaui) -lwayland-client -lwayland-egl -lEGL -lGL'
echo ""
echo -e "${GREEN}Happy coding!${NC}"
