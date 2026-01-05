# Building Lua Interpreter with EmmyLua Debugger Support

This directory contains build scripts to compile Lua interpreters that support EmmyLua debugging.

## Prerequisites

- CMake (version 3.10 or higher)
- C/C++ compiler (MSVC on Windows, GCC/Clang on Linux/macOS)
- Git (for cloning dependencies)

## Quick Start

### Windows (PowerShell)

Build Lua with specified version and configuration:

```powershell
# Build Lua 5.4 (Release)
.\build.ps1 -LuaVersion 54

# Build Lua 5.5 (Debug)
.\build.ps1 -LuaVersion 55 -BuildType Debug
```

### Linux / macOS (Bash)

Build Lua with specified version and configuration:

```bash
# Make the script executable (first time only)
chmod +x build.sh

# Build Lua 5.4 (Release)
./build.sh -v 54

# Build Lua 5.5 (Debug)
./build.sh -v 55 -t Debug
```

## Available Options

### Lua Versions
- `51` - Lua 5.1.5
- `52` - Lua 5.2.4
- `53` - Lua 5.3.5
- `54` - Lua 5.4.6
- `55` - Lua 5.5.0

### Build Types
- `Release` (default) - Optimized build
- `Debug` - Debug build with symbols

## Output

The compiled Lua interpreter will be installed in `Lua<version>` directory:
- `Lua51/` - Lua 5.1 binaries
- `Lua52/` - Lua 5.2 binaries
- `Lua53/` - Lua 5.3 binaries
- `Lua54/` - Lua 5.4 binaries
- `Lua55/` - Lua 5.5 binaries

## How It Works

The build process follows these steps:
1. Creates a build directory for the specified Lua version
2. Configures the project using CMake with `EMMY_LUA_VERSION` parameter
3. Builds the Lua DLL/shared library and executable
4. Installs the binaries to the output directory

The resulting Lua interpreter is linked with the necessary components to support EmmyLua debugging functionality.