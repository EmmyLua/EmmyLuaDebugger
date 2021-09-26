mkdir buildLua54
cd buildLua54
cmake .. -A x64 -DEMMY_LUA_VERSION=54
cmake --build . --config Debug
cmake --install . --config Debug --prefix ../Lua54
cd ..