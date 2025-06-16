<div align="center">

# 🚀 EmmyLua Debugger

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen?style=flat-square)](https://github.com/EmmyLua/EmmyLuaDebugger)
[![Lua Version](https://img.shields.io/badge/lua-5.1%20%7C%205.2%20%7C%205.3%20%7C%205.4%20%7C%20LuaJIT-orange?style=flat-square)](#lua-support)

**High-performance cross-platform Lua debugger with full debugging features including breakpoints, variable watch, stack trace, and more.**

</div>

---

## ✨ Features

- 🎯 **Full Debugging Features** - Breakpoints, step execution, variable watch, stack trace
- 🌍 **Cross-Platform Support** - Windows, macOS, Linux
- ⚡ **High Performance** - Efficient debugging communication based on TCP protocol
- 🔧 **Easy Integration** - Supports multiple Lua versions and game engine integration

## 🎮 Supported Platforms

| Platform      | Status | Notes                  |
|---------------|--------|------------------------|
| Windows x64   | ✅     | Fully supported        |
| macOS         | ✅     | Intel & Apple Silicon  |
| Linux         | ✅     | any                    |

## 🔧 Lua Support

| Lua Version | Status | Notes           |
|-------------|--------|-----------------|
| Lua 5.1     | ✅     | Fully supported |
| Lua 5.2     | ✅     | Fully supported |
| Lua 5.3     | ✅     | Fully supported |
| Lua 5.4     | ✅     | Fully supported |
| LuaJIT      | ✅     | Fully supported |

## 🚀 Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/EmmyLua/EmmyLuaDebugger.git
cd EmmyLuaDebugger
```

### 2. Build the Project

#### Windows (Visual Studio)
```bash
mkdir build
cd build
cmake .. -DEMMY_LUA_VERSION=54
cmake --build . --config Release
```

#### macOS/Linux
```bash
mkdir build
cd build
cmake .. -DEMMY_LUA_VERSION=54 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### 3. Basic Usage

#### Integrate the debugger in your Lua code:

```lua
-- Load the debugger module
local dbg = require('emmy_core')

-- Start the TCP debug server
dbg.tcpListen('localhost', 9966)

-- Wait for IDE connection
dbg.waitIDE()

-- Set a strong breakpoint here
dbg.breakHere()

-- Your Lua code
print("Hello, EmmyLua Debugger!")
```

#### Connect the debugger in your IDE:

1. Open an IDE that supports EmmyLua (e.g., IntelliJ IDEA + EmmyLua plugin)
2. Configure the debug connection: `TCP Connect` mode, address `localhost:9966`
3. Click to start debugging
4. Set breakpoints in your code and enjoy debugging!

## 📚 Documentation

## 🛠️ Build Options

| Option                | Default | Description                        |
|-----------------------|---------|------------------------------------|
| `EMMY_LUA_VERSION`    | `54`    | Lua version (51/52/53/54/jit)      |
| `EMMY_USE_LUA_SOURCE` | `OFF`   | Whether to build with Lua source   |

### Advanced Build Examples

```bash
# Build for a specific version
cmake .. -DEMMY_LUA_VERSION=53 
# Build using Lua source
cmake .. -DEMMY_USE_LUA_SOURCE=ON
```

### Development Environment Setup

1. Install required build tools:
    - Windows: Visual Studio 2019+
    - macOS: Xcode + Command Line Tools
    - Linux: GCC 7+ or Clang 6+

2. Install CMake 3.11+

3. Clone and build the project:
    ```bash
    git clone --recursive https://github.com/EmmyLua/EmmyLuaDebugger.git
    cd EmmyLuaDebugger
    mkdir build && cd build
    cmake ..
    cmake --build .
    ```

## 🙏 Acknowledgements

- [libuv](https://github.com/libuv/libuv) - Cross-platform asynchronous I/O library
- [nlohmann/json](https://github.com/nlohmann/json) - Modern C++ JSON library
- [Lua](https://www.lua.org/) - Powerful embedded scripting language

## 📞 Support & Contact

- 🐛 [Report Issues](https://github.com/EmmyLua/EmmyLuaDebugger/issues)
- 💬 [Discussion](https://github.com/EmmyLua/EmmyLuaDebugger/discussions)
---

<div align="center">

**⭐ If you find this project helpful, please give us a Star! ⭐**

Made with ❤️ by [EmmyLua Team](https://github.com/EmmyLua)

</div>