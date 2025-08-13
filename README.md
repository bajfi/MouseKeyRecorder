# MouseRecorder

A cross-platform mouse and keyboard event recording and playback application built with modern C++23 and Qt5.

## Features

- **Cross-Platform Support**: Works on Linux (X11) and Windows
- **Multiple File Formats**: Save recordings in JSON, XML, or binary format
- **Intelligent Recording**: Optional mouse movement optimization to reduce file size
- **Configurable Playback**: Variable speed playback with loop support
- **User-Friendly Interface**: Modern Qt5 GUI with tabbed interface
- **Extensible Architecture**: SOLID principles with clean separation of concerns
- **Comprehensive Testing**: Unit tests with GoogleTest framework
- **Detailed Logging**: Configurable logging with spdlog

## Architecture

The application follows a layered architecture with clear separation of concerns:

### Core Layer

- **Event System**: Type-safe event definitions with factory pattern
- **Interfaces**: Abstract interfaces for recording, playback, storage, and configuration
- **Configuration**: Thread-safe configuration management with change notifications

### Platform Layer

- **Linux Implementation**: X11-based event capture and replay using XInput2 and XTest
- **Windows Implementation**: Windows API-based event handling (planned)

### Storage Layer

- **JSON Storage**: Human-readable format using nlohmann/json
- **Binary Storage**: Compact format with optional compression
- **XML Storage**: Structured format using pugixml
- **Factory Pattern**: Automatic format detection and handler creation

### Application Layer

- **Main Application**: Orchestrates all components and manages lifecycle
- **Recording/Playback Sessions**: High-level business logic coordination

### Presentation Layer

- **Qt5 GUI**: Modern interface with .ui files for easy customization
- **Widget-based Design**: Modular UI components for recording, playback, and configuration

## Building

### Prerequisites

#### Ubuntu/Debian

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    qt5-default \
    qttools5-dev \
    libx11-dev \
    libxtst-dev \
    libxi-dev \
    libspdlog-dev \
    nlohmann-json3-dev \
    libpugixml-dev \
    libgtest-dev \
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
    spdlog-devel \
    json-devel \
    pugixml-devel \
    gtest-devel \
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
    spdlog \
    nlohmann-json \
    pugixml \
    gtest \
    pkgconf
```

### Build Instructions

1. **Clone the repository**

   ```bash
   git clone <repository-url>
   cd MouseRecorder
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
  -h, --help              Show help information
  -v, --version           Show version information
  -c, --config <file>     Specify configuration file path
  -l, --log-level <level> Set log level (trace, debug, info, warn, error, critical, off)
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

## Development

### Code Style

The project follows modern C++23 guidelines:

- SOLID principles
- RAII for resource management
- Smart pointers over raw pointers
- STL algorithms and containers
- Exception-based error handling
- Comprehensive const-correctness

### Testing

Run the test suite:

```bash
cd build
ctest --output-on-failure
```

Or run specific tests:

```bash
./MouseRecorderTests --gtest_filter="EventTest.*"
```

### Adding New Features

1. **New Event Types**: Extend the `Event` class and factory
2. **New Storage Formats**: Implement `IEventStorage` interface
3. **Platform Support**: Implement `IEventRecorder` and `IEventPlayer` interfaces
4. **GUI Components**: Create new Qt widgets with corresponding .ui files

### Architecture Guidelines

- Keep interfaces minimal and focused (Interface Segregation Principle)
- Use dependency injection for testability
- Separate platform-specific code into platform directories
- Add comprehensive unit tests for new functionality
- Follow the existing naming conventions and code style

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes with tests
4. Ensure all tests pass
5. Submit a pull request

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
