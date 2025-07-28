# -----------------------------------------------------------------------------
# DMC Project Build Configuration
# -----------------------------------------------------------------------------
# This is the Meson build configuration for the DMC project. It defines the
# project settings, source files, compilation flags, and platform-specific 
# configurations. The script supports different build types: Debug, Release, 
# and Profiling.
# -----------------------------------------------------------------------------

project(
  'DMC', 'cpp',
  version : '0.0.1',
  #DEBUG
  default_options : ['warning_level=3', 'cpp_std=c++17', 'debug=true', 'buildtype=debug']
  #RELEASE
  #default_options : ['warning_level=3', 'cpp_std=c++17', 'debug=false', 'buildtype=release', 'optimization=3']
  #PROFILE
  #default_options : ['warning_level=3', 'cpp_std=c++17', 'debug=true', 'buildtype=debug', 'b_ndebug=false', 'b_lto=false']
)

# Build type options
DEBUG = 1
RELEASE = 2
PROFILE = 3
TYPE = DEBUG  # Change this to RELEASE or PROFILE for different build types

# Source files for the project
sources = [
  '../src/main.cpp',
  '../src/jsonParser.cpp',
  '../src/filter.cpp',
  '../src/abstract.cpp'
]

# SIMD and CPU-specific options
SSE     = '-msse'
AVX     = '-mavx'
AVX512  = '-mavx512f'

# Default SIMD mode (can be adjusted for specific requirements)
SIMD_MODE = ''

# Common C++ compiler flags
cpp_args = [
  '-Wno-narrowing',              # Disable narrowing warnings (e.g., casting to a smaller type)
  '-Wshadow',
  '-Wdangling-reference',
  '-Wformat=2',
  '-Wnon-virtual-dtor',
  '-Wcast-align',
  '-Wunused',
  '-Wmisleading-indentation',
  '-Wduplicated-cond',
  '-Wduplicated-branches',
  '-Wlogical-op',
  '-Wnull-dereference',
  '-Wuseless-cast',
  '-Wdouble-promotion',
  '-Wimplicit-fallthrough'
  # '-fvisibility=hidden',         # Hide symbols by default
  # '-fPIC',                       # Generate position-independent code
  # '-fno-semantic-interposition', # Disable semantic interposition
]

# Platform-specific settings
# Check if we're on an Android environment
if host_machine.system() == 'android'
  message('Android environment detected: Disabling SIMD flags')
  # Disable SIMD flags on Android due to potential compatibility issues
else
  if SIMD_MODE != ''
    cpp_args += [SIMD_MODE]  # Only add SIMD_MODE if it's not empty
  endif
endif

# Linker arguments (e.g., libraries)
link_args = []

# For PGO, use '-fprofile-generate' to generate and use: '-fprofile-use=DMC.p/*.gcda', '-fprofile-correction' in compile_commands.json.

# Add custom flags based on build type
if TYPE == DEBUG
  cpp_args += ['-DDMC_DEBUG', '-Og', '-flto', '-fwhole-program']
  link_args += ['-flto', '-fwhole-program']
elif TYPE == RELEASE
  cpp_args += ['-DDMC_RELEASE', '-march=native', '-flto', '-fwhole-program']
  link_args += ['-flto', '-fwhole-program']
elif TYPE == PROFILE
  cpp_args += ['-DDMC_PROFILE', '-DDMC_DEBUG', '-pg']
  link_args += ['-pg']
endif

if TYPE == DEBUG or TYPE == PROFILE
  # Windows-specific settings (e.g., for debugging)
  if host_machine.system() == 'windows'
    link_args += ['-lDbgHelp', '-Wl,--export-all-symbols']    # Link with DbgHelp library for better debugging support
  elif host_machine.system() == 'linux'
    # Linux-specific settings can be added here if needed
    cpp_args += ['-rdynamic']  
  endif
endif

# Define the executables to be built
executable(
  'dmc',                      # Main executable
  sources,                     # Source files
  install : true,               # Install the executable after building
  cpp_args: cpp_args,          # C++ compiler flags
  link_args: link_args         # Linker flags
)

# Test executable
executable(
  'test_parser',               # Test executable name
  ['../test/test_parser.cpp', '../src/jsonParser.cpp'],
  install : false,              # Don't install test executable
  cpp_args: cpp_args,          # C++ compiler flags
  link_args: link_args         # Linker flags
)

# Filter test executable
executable(
  'test_filter',               # Filter test executable name
  ['../test/test_filter.cpp', '../src/filter.cpp'],
  install : false,              # Don't install test executable
  cpp_args: cpp_args,          # C++ compiler flags
  link_args: link_args         # Linker flags
)

# Abstract test executable
executable(
  'test_abstract',             # Abstract test executable name
  ['../test/test_abstract.cpp', '../src/abstract.cpp', '../src/filter.cpp', '../src/jsonParser.cpp'],
  install : false,              # Don't install test executable
  cpp_args: cpp_args,          # C++ compiler flags
  link_args: link_args         # Linker flags
)
