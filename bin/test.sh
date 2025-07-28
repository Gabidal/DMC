#!/bin/bash

# DMC Test Runner Script
# This script builds and runs all test executables in the DMC project
# and reports their pass/fail status.

set -e  # Exit on any error during build phase

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Script directory (should be the bin folder)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Test results tracking
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
declare -a TEST_RESULTS

# Function to print colored output
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

print_header() {
    echo -e "\n${BLUE}${BOLD}=== $1 ===${NC}"
}

print_separator() {
    echo -e "${YELLOW}$(printf '=%.0s' {1..60})${NC}"
}

# Function to run a test and capture results
run_test() {
    local test_name=$1
    local test_executable=$2
    local description=$3
    
    print_header "Running $test_name"
    echo "Description: $description"
    echo "Executable: $test_executable"
    echo ""
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # Run the test and capture both stdout and return code
    if cd "$BUILD_DIR" && timeout 30s "./$test_executable"; then
        print_status "$GREEN" "✓ $test_name PASSED"
        TEST_RESULTS+=("PASS: $test_name")
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        local exit_code=$?
        print_status "$RED" "✗ $test_name FAILED (exit code: $exit_code)"
        TEST_RESULTS+=("FAIL: $test_name (exit code: $exit_code)")
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    echo ""
}

# Function to build all targets
build_project() {
    print_header "Building DMC Project"
    
    cd "$SCRIPT_DIR"
    
    # Clean build (optional - uncomment if you want clean builds)
    # if [ -d "$BUILD_DIR" ]; then
    #     echo "Cleaning existing build directory..."
    #     rm -rf "$BUILD_DIR"
    # fi
    
    # Setup build directory if it doesn't exist
    if [ ! -d "$BUILD_DIR" ]; then
        echo "Setting up build directory..."
        meson setup build
    fi
    
    echo "Compiling all targets..."
    if meson compile -C build; then
        print_status "$GREEN" "✓ Build completed successfully"
        return 0
    else
        print_status "$RED" "✗ Build failed"
        return 1
    fi
}

# Function to check if executable exists and is runnable
check_executable() {
    local exe_name=$1
    local exe_path="$BUILD_DIR/$exe_name"
    
    if [ ! -f "$exe_path" ]; then
        print_status "$YELLOW" "⚠ Warning: $exe_name not found at $exe_path"
        return 1
    fi
    
    if [ ! -x "$exe_path" ]; then
        print_status "$YELLOW" "⚠ Warning: $exe_name is not executable"
        return 1
    fi
    
    return 0
}

# Function to display final results
show_results() {
    print_separator
    print_header "Test Results Summary"
    
    for result in "${TEST_RESULTS[@]}"; do
        if [[ $result == PASS* ]]; then
            print_status "$GREEN" "$result"
        else
            print_status "$RED" "$result"
        fi
    done
    
    echo ""
    print_status "$BLUE" "Total Tests: $TOTAL_TESTS"
    print_status "$GREEN" "Passed: $PASSED_TESTS"
    print_status "$RED" "Failed: $FAILED_TESTS"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        print_status "$GREEN" "${BOLD}🎉 ALL TESTS PASSED! 🎉"
        return 0
    else
        print_status "$RED" "${BOLD}❌ SOME TESTS FAILED ❌"
        return 1
    fi
}

# Main execution
main() {
    print_header "DMC Test Runner"
    echo "Building and testing all DMC components..."
    echo "Working directory: $SCRIPT_DIR"
    echo "Build directory: $BUILD_DIR"
    print_separator
    
    # Build the project first
    if ! build_project; then
        print_status "$RED" "Build failed. Cannot run tests."
        exit 1
    fi
    
    print_separator
    
    # Run filter tests (comprehensive unit tests)
    if check_executable "test_filter"; then
        run_test "Filter Unit Tests" "test_filter" "Tests for definition filtering functionality"
    fi
    
    # Run parser tests (JSON parsing and utility functions)
    if check_executable "test_parser"; then
        run_test "Parser Unit Tests" "test_parser" "Tests for JSON parsing functionality and utilities"
    fi
    
    print_separator
    
    # Show final results
    if show_results; then
        exit 0
    else
        exit 1
    fi
}

# Handle script arguments
case "${1:-}" in
    --help|-h)
        echo "DMC Test Runner"
        echo ""
        echo "Usage: $0 [OPTIONS]"
        echo ""
        echo "Options:"
        echo "  --help, -h    Show this help message"
        echo "  --clean       Clean build directory before building"
        echo "  --verbose     Enable verbose output"
        echo ""
        echo "This script builds all DMC targets and runs available tests."
        exit 0
        ;;
    --clean)
        if [ -d "$BUILD_DIR" ]; then
            echo "Cleaning build directory..."
            rm -rf "$BUILD_DIR"
        fi
        main
        ;;
    --verbose)
        set -x  # Enable verbose mode
        main
        ;;
    "")
        main
        ;;
    *)
        echo "Unknown option: $1"
        echo "Use --help for usage information"
        exit 1
        ;;
esac
