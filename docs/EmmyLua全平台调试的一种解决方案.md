[TOC]

# 如何利用超强[EmmyLuaDebugger](https://github.com/EmmyLua/EmmyLuaDebugger)实现全平台调试[Lua](https://www.lua.org/)代码?

这里给出UE4.26+lua大致的一种解决方案，具体有类似需要再实操一下。

## 流程

1.你需要能够完成emmy_core的创建注入_G环境中。

2.启动调试器服务器：`emmy_core.tcpListen('localhost', 9966)`。

3.IDE连接调试器：配置Tcp（IDE connect debugger），点击开始调试。

## 细节

### Q：重点是怎么注入emmy_core？

A1：对于Windows平台下的工程，直接使用现有的插件内置的emmy_core.dll

然后执行这段代码完成注入：（具体代码看调试器生成代码）

```lua
package.cpath = package.cpath .. ';C:/Users/XXX/AppData/Roaming/JetBrains/IntelliJIdea2023.2/plugins/EmmyLua/debugger/emmy/windows/x64/?.dll'
local dbg = require('emmy_core')
dbg.tcpListen('localhost', 9966)
```

A2：对于Windows平台打包的程序该如何注入emmy_core.dll呢？

这里以UnrealEngine为例，只需要把emmyluadebugger编译到插件，然后直接做一层绑定给lua调用即可。

A3：那么Windows平台都能编译emmy_core了，其他平台理论上也可以。（EmmyLuaDebugger依赖了一个重型C库：[libuv](https://github.com/libuv/libuv)。需要针对不同平台编译好这个库。）

注意事项：

0.这里需要结合自己工程内用到的lua源码参与编译，自己配置依赖，笔者用的[slua_unreal](https://github.com/Tencent/sluaunreal)+lua修改版，配置了slua_unreal依赖

1.需要改一下源代码每个文件加上编译宏ENABLE_EMMY_LUA_DEBUGGER，避免Shipping下参与编译

2.需要改一下源代码每个文件加上命名空间，例如我加了namespace Emmy

UnrealEngine Build脚本大概是这样：（自己配置一下路径就好了）

```c#
if (Target.Configuration != UnrealTargetConfiguration.Shipping && 
		    (Target.Platform == UnrealTargetPlatform.Win64 || 
		     Target.Platform == UnrealTargetPlatform.Android || 
		     Target.Platform == UnrealTargetPlatform.IOS ||
		     Target.Platform == UnrealTargetPlatform.Mac))
		{
			PrivateIncludePaths.AddRange(
				new string[]
				{
					Path.Combine(ModuleDirectory, "emmylua"),
					Path.Combine(ModuleDirectory, "emmylua/emmy_core"),
					Path.Combine(ModuleDirectory, "emmylua/emmy_core/src"),
					Path.Combine(ModuleDirectory, "emmylua/emmy_debugger"),
					Path.Combine(ModuleDirectory, "emmylua/emmy_debugger/include"),
					Path.Combine(ModuleDirectory, "emmylua/emmy_debugger/src"),
					Path.Combine(ModuleDirectory, "emmylua/third_party"),
					Path.Combine(ModuleDirectory, "emmylua/third_party/nlohmann"),
					Path.Combine(ModuleDirectory, "emmylua/third_party/nlohmann/include"),
					Path.Combine(ModuleDirectory, "emmylua/third_party/libuv1460"),
					Path.Combine(ModuleDirectory, "emmylua/third_party/libuv1460/include"),
				});
			bUseUnity = false;
			PublicDefinitions.Add("ENABLE_EMMY_LUA_DEBUGGER");
			PublicDefinitions.Add("EMMY_LUA_54");
			PublicDefinitions.Add("EMMY_USE_LUA_SOURCE");
			
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				PublicAdditionalLibraries.Add("Userenv.lib");
				PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "emmylua/third_party/libuv1460/lib/x64/libuv.lib"));
				PublicDefinitions.Add("EMMY_CORE_VERSION=\"Win64 DEV\"");
			}
			else if(Target.Platform == UnrealTargetPlatform.IOS)
			{
				PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "emmylua/third_party/libuv1460/lib/ios/libuv.a"));
				PublicDefinitions.Add("EMMY_CORE_VERSION=\"IOS DEV\"");
			}else if (Target.Platform == UnrealTargetPlatform.Android)
			{
				PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "emmylua/third_party/libuv1460/lib/android/arm64/libuv.a"));
				PublicDefinitions.Add("EMMY_CORE_VERSION=\"Android DEV\"");
			}else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "emmylua/third_party/libuv1460/lib/mac/libuv.a"));
				PublicDefinitions.Add("EMMY_CORE_VERSION=\"Mac DEV\"");
			}
		}
```

### Q：怎么多平台编译好这个libuv库呢？

A1：android libuv编译参考：[交叉编译 Android 的 libuv](https://fh0.github.io/%E7%BC%96%E8%AF%91/2019/12/16/android-libuv.html)

> 需要自己调整libuv版本到1.46，以及ndk级别到r21b（这个看具体项目，UE4.26用r21b）

A2：mac编译参考libuv官方编译脚本直接编译就好了：[libuv](https://github.com/libuv/libuv)

A3：ios平台的参考这个：[makios](https://gist.github.com/litesync/d7c484ccfd9552d88249ade4ebc69bba)

Q：IDEA怎么连接到Android平台下我程序内的debugger呢？

A：1.插入数据线；2.启动程序；3.程序完成debugger启动；4.adb端口映射：`adb forward tcp:9966 tcp:9966`；5.IDE点击调试。

Q：IDEA怎么连接ios平台下我程序内的debugger呢？

todo: ...