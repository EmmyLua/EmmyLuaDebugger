REM android_studio Sdk path need write for example: C:\Users\username\AppData\Local\Android\Sdk
set android_studio_sdk= 
set toolchain=%android_studio_sdk%\ndk-bundle\build\cmake\android.toolchain.cmake
set android_ndk=%android_studio_sdk%\ndk-bundle
set build_type=Release
set gernerator="Ninja"
if not exist %1 md %1
cd %1
cmake ../..  -DCMAKE_TOOLCHAIN_FILE=%toolchain% -DANDROID_NDK=%android_ndk% -DCMAKE_BUILD_TYPE=%build_type% -DANDROID_ABI="%1" -DCMAKE_GENERATOR=%gernerator% -DCMAKE_SYSTEM_NAME=Android -DANDROID_PLATFORM=android-28 -DEMMY_USE_LUA_SOURCE=ON