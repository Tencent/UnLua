# 控制台命令

## 概述

在游戏运行的时候直接执行一些调试命令，比如调用特定对象上的方法、修改属性等等，可以为我们调试游戏环境提供一些便利。

默认按 `~` 键可以呼出引擎自带的命令控制台，UnLua自带了一些基础的命令，如果你有一些好的想法，也可以参考 `UnLuaConsoleCommands` 的实现来扩展。

## 命令清单

### lua.do \<code\>

在默认环境执行一段Lua代码。

示例：
```
lua.do print("hello world")
```

### lua.dofile \<module path\>

在默认环境执行指定的Lua文件。

示例：
```
lua.do test1.test2
```

### lua.gc

在默认环境强制执行一次垃圾回收。
