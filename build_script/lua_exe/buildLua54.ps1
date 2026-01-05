mkdir buildLua54
cd buildLua54
cmake .. -A x64 -DEMMY_LUA_VERSION=54
cmake --build . --config Release
cmake --install . --config Release --prefix ../Lua54
cd ..