#!/bin/bash

# MouseRecorder Version Management Script
# This script helps manage versions for the MouseRecorder project

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && cd .. && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

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

# Function to get current version from CMakeLists.txt
get_current_version() {
    grep -A1 "project(MouseRecorder" "$PROJECT_ROOT/CMakeLists.txt" | grep "VERSION" | sed 's/.*VERSION \([0-9.]*\).*/\1/'
}

# Function to update version in CMakeLists.txt
update_version_in_cmake() {
    local new_version="$1"
    print_status "Updating version to $new_version in CMakeLists.txt..."

    sed -i "/project(MouseRecorder/,/)/s/VERSION [0-9.]*/VERSION $new_version/" "$PROJECT_ROOT/CMakeLists.txt"

    if [ $? -eq 0 ]; then
        print_success "Version updated successfully in CMakeLists.txt"
    else
        print_error "Failed to update version in CMakeLists.txt"
        exit 1
    fi
}

# Function to create git tag
create_git_tag() {
    local version="$1"
    local tag_name="v$version"

    print_status "Creating git tag $tag_name..."

    # Check if tag already exists
    if git tag -l | grep -q "^$tag_name$"; then
        print_warning "Tag $tag_name already exists"
        return 1
    fi

    git tag -a "$tag_name" -m "Release version $version"

    if [ $? -eq 0 ]; then
        print_success "Git tag $tag_name created successfully"
        print_status "To push the tag, run: git push origin $tag_name"
    else
        print_error "Failed to create git tag"
        return 1
    fi
}

# Function to display current version information
show_version_info() {
    print_status "Current version information:"
    echo

    local current_version=$(get_current_version)
    echo "  CMakeLists.txt version: $current_version"

    if [ -d "$PROJECT_ROOT/.git" ]; then
        echo "  Git information:"
        echo "    Current commit: $(git rev-parse --short HEAD 2>/dev/null || echo 'N/A')"
        echo "    Current branch: $(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'N/A')"
        echo "    Working tree: $(if git diff-index --quiet HEAD -- 2>/dev/null; then echo 'clean'; else echo 'dirty'; fi)"

        local latest_tag=$(git describe --tags --abbrev=0 2>/dev/null || echo 'none')
        echo "    Latest tag: $latest_tag"
    else
        echo "  Git: Not a git repository"
    fi

    if [ -f "$PROJECT_ROOT/build/generated/version.hpp" ]; then
        echo
        echo "  Generated version information:"
        grep "VERSION_STRING\|VERSION_FULL" "$PROJECT_ROOT/build/generated/version.hpp" | sed 's/^/    /'
    fi
}

# Function to bump version
bump_version() {
    local bump_type="$1"
    local current_version=$(get_current_version)

    # Parse current version
    IFS='.' read -ra VERSION_PARTS <<< "$current_version"
    local major=${VERSION_PARTS[0]:-0}
    local minor=${VERSION_PARTS[1]:-0}
    local patch=${VERSION_PARTS[2]:-0}

    case "$bump_type" in
        major)
            major=$((major + 1))
            minor=0
            patch=0
            ;;
        minor)
            minor=$((minor + 1))
            patch=0
            ;;
        patch)
            patch=$((patch + 1))
            ;;
        *)
            print_error "Invalid bump type. Use: major, minor, or patch"
            exit 1
            ;;
    esac

    local new_version="$major.$minor.$patch"
    echo "$new_version"
}

# Function to show usage
show_help() {
    echo "MouseRecorder Version Management Script"
    echo
    echo "Usage: $0 [command] [options]"
    echo
    echo "Commands:"
    echo "  info                    Show current version information"
    echo "  set <version>          Set version to specific value (e.g., 1.2.3)"
    echo "  bump <type>            Bump version (major|minor|patch)"
    echo "  tag [version]          Create git tag (uses current version if not specified)"
    echo "  release <type>         Bump version and create git tag"
    echo "  help                   Show this help message"
    echo
    echo "Examples:"
    echo "  $0 info                # Show current version info"
    echo "  $0 set 1.0.0          # Set version to 1.0.0"
    echo "  $0 bump patch         # Bump patch version (1.0.0 -> 1.0.1)"
    echo "  $0 bump minor         # Bump minor version (1.0.1 -> 1.1.0)"
    echo "  $0 bump major         # Bump major version (1.1.0 -> 2.0.0)"
    echo "  $0 tag                # Create tag for current version"
    echo "  $0 tag 1.0.0          # Create tag for specific version"
    echo "  $0 release patch      # Bump patch and create tag"
}

# Main script logic
case "${1:-}" in
    info|"")
        show_version_info
        ;;
    set)
        if [ -z "$2" ]; then
            print_error "Please specify a version (e.g., 1.2.3)"
            exit 1
        fi

        # Validate version format
        if ! echo "$2" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
            print_error "Invalid version format. Use semantic versioning (e.g., 1.2.3)"
            exit 1
        fi

        update_version_in_cmake "$2"
        print_success "Version set to $2"
        ;;
    bump)
        if [ -z "$2" ]; then
            print_error "Please specify bump type: major, minor, or patch"
            exit 1
        fi

        new_version=$(bump_version "$2")
        update_version_in_cmake "$new_version"
        print_success "Version bumped to $new_version"
        ;;
    tag)
        version="${2:-$(get_current_version)}"
        create_git_tag "$version"
        ;;
    release)
        if [ -z "$2" ]; then
            print_error "Please specify release type: major, minor, or patch"
            exit 1
        fi

        print_status "Creating release with $2 version bump..."

        new_version=$(bump_version "$2")
        update_version_in_cmake "$new_version"
        print_success "Version bumped to $new_version"

        create_git_tag "$new_version"

        print_success "Release $new_version completed!"
        print_status "Don't forget to:"
        print_status "  1. Commit the version change: git add CMakeLists.txt && git commit -m 'Bump version to $new_version'"
        print_status "  2. Push the changes: git push origin main"
        print_status "  3. Push the tag: git push origin v$new_version"
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        print_error "Unknown command: $1"
        echo
        show_help
        exit 1
        ;;
esac
