mkdir buildLua53
cd buildLua53
cmake .. -A x64 -DEMMY_LUA_VERSION=53
cmake --build . --config Debug
cmake --install . --config Debug --prefix ../Lua53
cd ..