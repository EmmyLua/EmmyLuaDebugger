mkdir buildLua51
cd buildLua51
cmake .. -A x64 -DEMMY_LUA_VERSION=51
cmake --build . --config Debug
cmake --install . --config Debug --prefix ../Lua51
cd ..