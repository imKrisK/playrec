# Contributing to PlayRec

Thank you for your interest in contributing to PlayRec! This document provides guidelines and information for contributors.

## 🎯 Areas Where We Need Help

### High Priority
1. **Windows Implementation**
   - DXGI Desktop Duplication API integration
   - WASAPI audio capture implementation
   - Windows hardware encoder support (NVENC, Quick Sync)

2. **Linux Implementation**
   - X11 screen capture with XDamage optimization
   - ALSA/PulseAudio audio capture
   - VA-API hardware acceleration

3. **Advanced Features**
   - Real-time MP4 muxing with proper timestamps
   - Hardware acceleration detection and fallback
   - Error handling and recovery mechanisms

### Medium Priority
- Documentation improvements
- Performance optimizations
- Cross-platform testing
- Example applications and tutorials

## 🚀 Getting Started

### Development Environment Setup

#### Prerequisites
- C++17 compatible compiler
- CMake 3.16+
- Git
- Platform-specific development tools

#### macOS Setup
```bash
# Install development tools
xcode-select --install

# Install dependencies
brew install ffmpeg pkg-config cmake

# Clone and build
git clone <repository-url>
cd playrec
./build.sh
```

#### Windows Setup
```bash
# Install dependencies using vcpkg
vcpkg install ffmpeg:x64-windows

# Or use Chocolatey
choco install cmake git visualstudio2022community
```

#### Linux Setup
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential cmake git pkg-config
sudo apt install libavcodec-dev libavformat-dev libavutil-dev
sudo apt install libx11-dev libasound2-dev

# Fedora/RHEL
sudo dnf install gcc-c++ cmake git pkg-config
sudo dnf install ffmpeg-devel libX11-devel alsa-lib-devel
```

### Building the Project
```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
cmake --build . --config Release

# Test
./bin/PlayRec --help
```

## 📝 Code Guidelines

### Code Style
- **Language**: Modern C++17
- **Naming**: snake_case for variables/functions, PascalCase for classes
- **Headers**: Include guards with #pragma once
- **Memory Management**: Use smart pointers (std::unique_ptr, std::shared_ptr)
- **Error Handling**: Use exceptions for error conditions
- **Threading**: Use std::thread and synchronization primitives

### Example Code Structure
```cpp
#pragma once

#include <memory>
#include <vector>

namespace playrec {

class VideoCapture {
public:
    VideoCapture();
    virtual ~VideoCapture();
    
    virtual bool initialize(const CaptureSettings& settings) = 0;
    virtual std::unique_ptr<Frame> capture_frame() = 0;
    virtual void cleanup() = 0;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace playrec
```

### Platform-Specific Implementation
Each platform should implement the abstract interfaces:

```cpp
// Windows implementation
class WindowsVideoCapture : public VideoCapture {
    // DXGI implementation
};

// macOS implementation  
class MacOSVideoCapture : public VideoCapture {
    // CoreGraphics implementation
};

// Linux implementation
class LinuxVideoCapture : public VideoCapture {
    // X11 implementation
};
```

## 🔄 Development Workflow

### 1. Issue Creation
- Check existing issues to avoid duplicates
- Use issue templates when available
- Provide clear description and reproduction steps
- Label appropriately (bug, feature, enhancement)

### 2. Branch Strategy
```bash
# Create feature branch from main
git checkout -b feature/windows-dxgi-capture

# Or fix branch for bugs
git checkout -b fix/audio-sync-issue
```

### 3. Implementation
- Follow existing code patterns
- Add appropriate error handling
- Include unit tests when possible
- Update documentation

### 4. Testing
```bash
# Build and test
./build.sh
./build/bin/PlayRec --help

# Test specific features
./build/bin/PlayRec --codec h264 --fps 30 --output test.mp4
./build/bin/PlayRec --codec h265 --quality high --output test_h265.mp4
```

### 5. Pull Request
- Create descriptive PR title and description
- Reference related issues
- Include testing information
- Update README if needed

## 🧪 Testing

### Manual Testing Checklist
- [ ] Application builds without errors
- [ ] Help command shows correct options
- [ ] H.264 encoding works
- [ ] H.265 encoding works (if supported)
- [ ] Audio capture functions
- [ ] Output files are valid
- [ ] Hardware acceleration detected (if available)

### Platform-Specific Testing
#### macOS
- [ ] CoreGraphics capture works
- [ ] CoreAudio capture works
- [ ] VideoToolbox acceleration works
- [ ] ARM optimization active

#### Windows (when implemented)
- [ ] DXGI capture works
- [ ] WASAPI audio works
- [ ] NVENC/QuickSync detected

#### Linux (when implemented)
- [ ] X11 capture works
- [ ] ALSA/PulseAudio works
- [ ] VA-API acceleration works

## 📚 Documentation

### Code Documentation
- Use clear, descriptive comments
- Document public APIs with doxygen-style comments
- Include usage examples for complex functions

### README Updates
- Update feature status when implementing new platforms
- Add new command-line options
- Update build instructions for new dependencies

## 🐛 Bug Reports

### Information to Include
1. **Environment**
   - Operating system and version
   - Compiler and version
   - FFmpeg version
   - Hardware specifications

2. **Steps to Reproduce**
   - Exact command line used
   - Input files or settings
   - Expected vs actual behavior

3. **Logs and Output**
   - Error messages
   - Console output
   - Log files if available

### Example Bug Report
```
**Environment:**
- macOS 13.0 (Apple Silicon M2)
- Clang 14.0.3
- FFmpeg 5.1.2
- 16GB RAM

**Steps to Reproduce:**
1. Run: ./PlayRec --codec h265 --fps 60 --output test.mp4
2. Let it run for 30 seconds
3. Press Enter to stop

**Expected:** Clean exit with valid MP4 file
**Actual:** Segmentation fault on exit

**Output:**
[Include relevant console output]
```

## 🎯 Platform Implementation Guidelines

### Windows DXGI Implementation
```cpp
// Key components needed:
// 1. IDXGIOutputDuplication for screen capture
// 2. WASAPI for audio capture  
// 3. Media Foundation for hardware encoding
// 4. Error handling for display changes

class WindowsVideoCapture : public VideoCapture {
private:
    ComPtr<IDXGIOutputDuplication> m_duplication;
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
};
```

### Linux X11 Implementation
```cpp
// Key components needed:
// 1. XGetImage for screen capture
// 2. XDamage for change detection
// 3. ALSA/PulseAudio for audio
// 4. VA-API for hardware encoding

class LinuxVideoCapture : public VideoCapture {
private:
    Display* m_display;
    Window m_root_window;
    Damage m_damage;
};
```

## 🏆 Recognition

Contributors will be recognized in:
- README.md contributors section
- Release notes for significant contributions
- GitHub contributor statistics

## 📞 Getting Help

- **Discussions**: Use GitHub Discussions for questions
- **Issues**: Use GitHub Issues for bugs and feature requests
- **Code Review**: Tag maintainers for PR reviews

Thank you for contributing to PlayRec! 🎮