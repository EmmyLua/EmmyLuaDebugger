如果希望将emmylua 调试器静态集成进项目（比如远程调试手机）可以参考这里

这里以xlua为例
1.在下载xlua源代码
2.将emmylua调试器目录复制到xlua源代码build目录下取名EmmyDebugger
3.到build目录下打开CMakeLists.txt在最下面加上

add_subdirectory(EmmyDebugger/build_script/xlua_emmy_core)
add_dependencies(xlua emmy_core)
target_link_libraries(xlua PRIVATE emmy_core)

之后就可以通过xlua的各种构建脚本编译出对应的带emmy_core 调试器的xlua了
5.构建完之后复制xlua.dll/libxlua.so到unity对应的目录中
6.在自己的C#代码中加上

```C#

namespace LuaDLL
{ 
    public partial class Lua
    { 
        [DllImport("xlua", CallingConvention = CallingConvention.Cdecl)]
        public static extern int luaopen_emmy_core(System.IntPtr L);

        [MonoPInvokeCallback(typeof(lua_CSFunction))]
        public static int LoadEmmyCore(System.IntPtr L)
        {
            return luaopen_emmy_core(L);
        }
    }
}
```

并且在创建LuaEnv实例之后加上

luaEnv.AddBuildin("emmy_core",LuaDLL.Lua.LoadEmmyCore);

就可以在其他平台使用emmy_core调试了