# MouseRecorder Version Management

This document describes the automated version management system implemented for the MouseRecorder project.

## How It Works

### Version Generation Process

1. **CMake Configuration**: When you run `cmake ..`, the version system:
   - Reads the version from `CMakeLists.txt`
   - Extracts git information (commit hash, branch, dirty state)
   - Generates `build/generated/version.hpp` from `src/version.hpp.in`
   - Generates `build/mouserecorder.desktop` from `mouserecorder.desktop.in`

2. **Build Time**: The generated header is included by source files that need version information

3. **Runtime**: Applications can access version information through the `MouseRecorder::Version` namespace

### Version Format

- **Clean version**: `1.2.3` (when on a git tag with clean working tree)
- **Development version**: `1.2.3-g1234567` (when not on a tag)
- **Dirty version**: `1.2.3-dirty` or `1.2.3-g1234567-dirty` (when working tree has uncommitted changes)

## Files Involved

### Template Files

- `src/version.hpp.in` - C++ header template
- `mouserecorder.desktop.in` - Desktop file template

### Generated Files

- `build/generated/version.hpp` - Generated C++ header
- `build/mouserecorder.desktop` - Generated desktop file

### Configuration Files

- `CMakeLists.txt` - Contains the authoritative version number
- `cmake/version.cmake` - Version generation logic

### Source Files Using Version

- `src/main.cpp` - Sets Qt application version
- `src/application/MouseRecorderApp.cpp` - Provides getVersion() method
- `src/core/IEventStorage.hpp` - Default metadata version

## Usage

### Manual Version Management

To change the version manually, edit the `VERSION` field in the `project()` declaration in `CMakeLists.txt`:

```cmake
project(MouseRecorder
    VERSION 1.2.3  # <-- Change this
    DESCRIPTION "Cross-platform Mouse and Keyboard Event Recorder"
    LANGUAGES CXX
)
```

After changing the version, rebuild the project to update all generated files.

### Using the Version Management Script

The `version.sh` script provides convenient commands for version management:

#### Show Current Version Information

```bash
./version.sh info
```

#### Set Specific Version

```bash
./version.sh set 1.0.0
```

#### Bump Version

```bash
./version.sh bump patch    # 1.0.0 -> 1.0.1
./version.sh bump minor    # 1.0.1 -> 1.1.0
./version.sh bump major    # 1.1.0 -> 2.0.0
```

#### Create Git Tag

```bash
./version.sh tag           # Tag current version
./version.sh tag 1.0.0     # Tag specific version
```

#### Complete Release Process

```bash
./version.sh release patch  # Bump patch version and create tag
./version.sh release minor  # Bump minor version and create tag
./version.sh release major  # Bump major version and create tag
```

## Release Workflow

### For Patch Releases (Bug fixes)

```bash
# 1. Bump version and create tag
./version.sh release patch

# 2. Commit the version change
git add CMakeLists.txt
git commit -m "Bump version to $(./version.sh info | grep "CMakeLists.txt version" | cut -d: -f2 | tr -d ' ')"

# 3. Push changes and tag
git push origin main
git push origin v$(./version.sh info | grep "CMakeLists.txt version" | cut -d: -f2 | tr -d ' ')
```

### For Minor/Major Releases

Follow the same process but use `./version.sh release minor` or `./version.sh release major`.
