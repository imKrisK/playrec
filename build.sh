#!/bin/bash

# Build script for PlayRec
# This script handles the build process for all platforms

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if cmake is installed
if ! command -v cmake &> /dev/null; then
    print_error "CMake is not installed. Please install CMake 3.16 or higher."
    exit 1
fi

# Check CMake version
CMAKE_VERSION=$(cmake --version | head -n1 | cut -d" " -f3)
print_status "Found CMake version: $CMAKE_VERSION"

# Detect platform
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="Linux"
    BUILD_TYPE="Unix Makefiles"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
    BUILD_TYPE="Unix Makefiles"
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    PLATFORM="Windows"
    BUILD_TYPE="Visual Studio 16 2019"
else
    PLATFORM="Unknown"
    BUILD_TYPE="Unix Makefiles"
fi

print_status "Detected platform: $PLATFORM"

# Parse command line arguments
BUILD_CONFIG="Release"
CLEAN_BUILD=false
INSTALL_DEPS=false
RUN_TESTS=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_CONFIG="Debug"
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --install-deps)
            INSTALL_DEPS=true
            shift
            ;;
        --test)
            RUN_TESTS=true
            shift
            ;;
        --help|-h)
            echo "PlayRec Build Script"
            echo ""
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --debug        Build in Debug mode (default: Release)"
            echo "  --clean        Clean build directory before building"
            echo "  --install-deps Install platform-specific dependencies"
            echo "  --test         Run tests after building"
            echo "  --help, -h     Show this help message"
            echo ""
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information."
            exit 1
            ;;
    esac
done

print_status "Build configuration: $BUILD_CONFIG"

# Install dependencies if requested
if [ "$INSTALL_DEPS" = true ]; then
    print_status "Installing platform-specific dependencies..."
    
    case $PLATFORM in
        "Linux")
            print_status "Installing Linux dependencies..."
            if command -v apt-get &> /dev/null; then
                sudo apt-get update
                sudo apt-get install -y build-essential cmake libx11-dev libasound2-dev
            elif command -v yum &> /dev/null; then
                sudo yum groupinstall -y "Development Tools"
                sudo yum install -y cmake libX11-devel alsa-lib-devel
            elif command -v pacman &> /dev/null; then
                sudo pacman -S --needed base-devel cmake libx11 alsa-lib
            else
                print_warning "Package manager not detected. Please install dependencies manually."
            fi
            ;;
        "macOS")
            print_status "Installing macOS dependencies..."
            if command -v brew &> /dev/null; then
                brew install cmake
            else
                print_warning "Homebrew not found. Please install Homebrew or dependencies manually."
            fi
            ;;
        "Windows")
            print_status "Windows dependencies should be installed via Visual Studio installer."
            ;;
    esac
fi

# Create and enter build directory
BUILD_DIR="build"

if [ "$CLEAN_BUILD" = true ] && [ -d "$BUILD_DIR" ]; then
    print_status "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

if [ ! -d "$BUILD_DIR" ]; then
    print_status "Creating build directory..."
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure project with CMake
print_status "Configuring project with CMake..."
if [[ "$PLATFORM" == "Windows" ]]; then
    cmake .. -G "$BUILD_TYPE" -A x64
else
    cmake .. -DCMAKE_BUILD_TYPE="$BUILD_CONFIG"
fi

if [ $? -ne 0 ]; then
    print_error "CMake configuration failed!"
    exit 1
fi

print_success "CMake configuration completed successfully!"

# Build the project
print_status "Building project..."
cmake --build . --config "$BUILD_CONFIG" --parallel

if [ $? -ne 0 ]; then
    print_error "Build failed!"
    exit 1
fi

print_success "Build completed successfully!"

# Check if executable was created
if [[ "$PLATFORM" == "Windows" ]]; then
    EXECUTABLE="bin/$BUILD_CONFIG/PlayRec.exe"
else
    EXECUTABLE="bin/PlayRec"
fi

if [ -f "$EXECUTABLE" ]; then
    print_success "Executable created: $EXECUTABLE"
    
    # Show executable info
    if [[ "$PLATFORM" != "Windows" ]]; then
        print_status "Executable size: $(ls -lh "$EXECUTABLE" | cut -d' ' -f5)"
    fi
else
    print_warning "Executable not found at expected location: $EXECUTABLE"
fi

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    print_status "Running tests..."
    if [ -f "$EXECUTABLE" ]; then
        ./"$EXECUTABLE" --help
        print_success "Basic test completed successfully!"
    else
        print_error "Cannot run tests - executable not found!"
        exit 1
    fi
fi

# Final instructions
print_success "Build process completed!"
echo ""
print_status "To run PlayRec:"
echo "  cd build"
echo "  ./$EXECUTABLE --help"
echo ""
print_status "To install system-wide (optional):"
echo "  sudo cmake --install . --config $BUILD_CONFIG"
echo ""

exit 0