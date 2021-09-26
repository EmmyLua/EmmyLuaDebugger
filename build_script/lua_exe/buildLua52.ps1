mkdir buildLua52
cd buildLua52
cmake .. -A x64 -DEMMY_LUA_VERSION=52
cmake --build . --config Debug
cmake --install . --config Debug --prefix ../Lua52
cd ..