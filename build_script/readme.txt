先在android.bat 中设置android_studio_sdk的位置然后
cd android
然后用以下命令创建构造目标
android.bat arm64-v8a
android.bat armeabi-v7a
android.bat x86
创建完成之后 cd到其中一个目标去然后
ninja -f build.ninja
libemmy_core.so就生成了
