#!/bin/bash

# =============================================================================
# dmc Project Initialization Script
# =============================================================================
# This script initializes and builds the dmc project locally. It validates
# the environment, checks for required tools, manages the build setup, and
# ensures analytics tools are properly configured.
#
# Author: DMC Development Team
# =============================================================================

# Function to check if the script is being run from the project root directory
check_directory() {
    local current_dir=$(pwd)
    local project_root_name="DMC"
    local bin_dir_name="bin"

    # Attempt to find the project root directory by looking for the .git directory
    project_root=$(git rev-parse --show-toplevel 2>/dev/null)

    # If the project root is not found, exit with an error message
    if [ -z "$project_root" ]; then
        echo "Error: Unable to determine the project root directory. Ensure you're in the dmc project."
        exit 1
    fi

    # Navigate to the 'bin' directory if not already there
    if [ "$(basename "$project_root")" == "$project_root_name" ] && [ "$current_dir" == "$project_root" ]; then
        echo "Project root directory detected. Changing to the 'bin' directory."
        cd "$project_root/$bin_dir_name" || exit 1
    elif [[ "$current_dir" != "$project_root/$bin_dir_name"* ]]; then
        echo "Navigating to the 'bin' directory within the project."
        cd "$project_root/$bin_dir_name" || exit 1
    fi
}

# Function to check if a command is available and exit if not
check_command() {
    local cmd="$1"
    local cmd_name="$2"

    if ! command -v "$cmd" &>/dev/null; then
        echo "Error: '$cmd_name' is required but not installed. Please install it before running this script."
        exit 1
    fi
}

# Check if we're in the correct directory
check_directory

echo "=== dmc Project Initialization ==="
echo

# Step 1: Run analytics validation
echo "Step 1: Validating environment and analytics tools..."
if [[ -f "./analytics/utils/validate.sh" ]]; then
    if bash "./analytics/utils/validate.sh"; then
        echo "✓ Environment validation completed successfully."
    else
        echo "✗ Environment validation failed. Please address the issues above."
        echo "You can continue with basic build, but analytics tools may not work properly."
        read -p "Continue anyway? [y/N]: " -r
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Initialization cancelled."
            exit 1
        fi
    fi
else
    echo "Warning: Analytics validation script not found. Continuing with basic checks..."
    # Ensure the required tools are installed
    check_command "meson" "meson"
    check_command "g++" "g++"
fi

echo

# Step 2: Set up build environment
echo "Step 2: Setting up build environment..."

# Ensure the CXX variable is set (default to 'g++' if not)
if [ -z "$CXX" ]; then
    export CXX=g++
    echo "CXX environment variable was not set. Defaulting to 'g++'."
fi

# Step 3: Set up the build directory (wipe existing if necessary)
echo "Step 3: Setting up the build directory..."
if [ -d "./build" ]; then
    echo "Existing build directory found. Cleaning up..."
    meson setup --wipe build || exit 1
else
    echo "Creating new build directory..."
    meson setup build || exit 1
fi

# Step 4: Compile the project using meson
echo "Step 4: Compiling the project..."
meson compile -C build || exit 1

# Step 5: Set up analytics tools
echo "Step 5: Setting up analytics tools..."

echo
echo "=== Initialization Complete ==="
echo "✓ Build process completed successfully!"
echo "✓ dmc executable: ./build/dmc"
