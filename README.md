# MouseRecorder

[![build](https://img.shields.io/github/actions/workflow/status/USERNAME/MouseRecorder/ci.yml?branch=main&label=build&logo=github)](https://github.com/bajfi/MouseKeyRecorder/actions/workflows/ci.yml)
[![Ubuntu](https://img.shields.io/badge/Ubuntu-passing-brightgreen?logo=ubuntu)](https://github.com/bajfi/MouseKeyRecorder/actions/workflows/ci.yml)
[![code quality](https://img.shields.io/badge/code%20quality-A-brightgreen?logo=codeclimate)](https://github.com/bajfi/MouseKeyRecorder/actions/workflows/code-quality.yml)
[![license](https://img.shields.io/badge/license-MIT-blue?logo=opensource)](LICENSE)

A cross-platform mouse and keyboard event recording and playback application built with modern C++23 and Qt.

## Features

- **Cross-Platform Support**: Works on Linux (X11) with Windows and macOS support planned
- **Multiple File Formats**: Save recordings in JSON, XML, or binary format (.mre)
- **Intelligent Recording**: Optional mouse movement optimization to reduce file size
- **Configurable Playback**: Variable speed playback with loop support
- **User-Friendly Interface**: Modern Qt5/Qt6 GUI with tabbed interface
- **Extensible Architecture**: SOLID principles with clean separation of concerns
- **Comprehensive Testing**: Unit tests with GoogleTest framework and CI/CD pipeline
- **Detailed Logging**: Configurable logging with spdlog

## Building

### Prerequisites

#### Ubuntu/Debian

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    qtbase5-dev \
    qtchooser \
    qt5-qmake \
    qtbase5-dev-tools \
    qtcreator \
    libx11-dev \
    libxtst-dev \
    libxi-dev \
    pkg-config
```

#### Fedora/RHEL/CentOS

```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    qt5-qtbase-devel \
    qt5-qttools-devel \
    libX11-devel \
    libXtst-devel \
    libXi-devel \
    pkgconfig
```

#### Arch Linux

```bash
sudo pacman -S \
    base-devel \
    cmake \
    qt5-base \
    qt5-tools \
    libx11 \
    libxtst \
    libxi \
    pkgconf
```

### Build Instructions

1. **Clone the repository**

   ```bash
   git clone https://github.com/bajfi/MouseKeyRecorder.git
   cd MouseKeyRecorder
   ```

2. **Create build directory**

   ```bash
   mkdir build
   cd build
   ```

3. **Configure with CMake**

   ```bash
   cmake -DCMAKE_BUILD_TYPE=Release ..
   ```

4. **Build the application**

   ```bash
   make -j$(nproc)
   ```

5. **Run tests (optional)**

   ```bash
   ctest --output-on-failure
   ```

6. **Install (optional)**

   ```bash
   sudo make install
   ```

## Usage

### Basic Usage

1. **Launch the application**

   ```bash
   ./MouseRecorder
   ```

2. **Recording Events**
   - Go to the "Recording" tab
   - Configure capture options (mouse/keyboard events)
   - Click "Start Recording"
   - Perform the actions you want to record
   - Click "Stop Recording"
   - Export the events to a file

3. **Playing Back Events**
   - Go to the "Playback" tab
   - Load a recording file using "Browse..."
   - Adjust playback speed if needed
   - Click "Play" to start playback
   - Use "Pause" or "Stop" to control playback

4. **Configuration**
   - Go to the "Configuration" tab
   - Adjust recording settings, playback options, shortcuts, and system settings
   - Click "Apply" to save changes

### Command Line Options

```bash
./MouseRecorder [options]

Options:
  -h, --help                Show help information
  -v, --version             Show version information
  -c, --config <file>       Specify configuration file path
  -l, --log-level <level>   Set log level (trace, debug, info, warn, error, critical, off)
```

### File Formats

#### JSON Format (.json)

- Human-readable text format
- Easy to inspect and modify
- Larger file size
- Good for debugging and manual editing

#### Binary Format (.mre)

- Compact binary format
- Smallest file size
- Optional compression
- Best for large recordings

#### XML Format (.xml)

- Structured markup format
- Human-readable but verbose
- Good for integration with other tools

### Configuration

The application stores configuration in:

- Linux: `~/.config/mouserecorder.conf`
- Windows: `%APPDATA%/mouserecorder.conf`

Key configuration options:

- **Recording**: Event capture settings, movement optimization
- **Playback**: Default speed, loop settings
- **Shortcuts**: Customizable keyboard shortcuts
- **System**: Logging levels and file paths

## Contributing

We welcome contributions! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes with comprehensive tests
4. Ensure all CI checks pass (build, tests, code quality)
5. Submit a pull request with a clear description

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Dependencies

- **Qt5**: GUI framework
- **spdlog**: Logging library
- **nlohmann/json**: JSON handling
- **pugixml**: XML parsing
- **GoogleTest**: Testing framework
- **X11/XInput2/XTest**: Linux event handling
- **Windows API**: Windows event handling (future)

## Platform Support

### Linux (Current)

- X11-based event capture and replay
- Requires XInput2 and XTest extensions
- Tested on Ubuntu 20.04+, Fedora 35+, Arch Linux

### Windows (Planned)

- Windows API-based event handling
- Support for Windows 10/11

### macOS (Future)

- Core Graphics and Quartz Event Services
- macOS 10.15+
