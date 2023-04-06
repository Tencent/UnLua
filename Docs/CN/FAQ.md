# FAQ 常见问题解答

## 覆写RecieveTick无效？

只有当蓝图中的Tick事件启用时，Lua中的Tick才会执行。这个是UE自己的设计，开启Tick有两种方法，一种是在C++中把 `bCanEverTick` 设为true，另一种就是在蓝图中创建 `Event Tick` 节点，可见 `FKismetCompilerContext::SetCanEverTick()` 函数。

## Lua代码打包后加载不到？

在 "Additional Non-Asset Directories to Package" 一栏中加入Script目录。

## 蓝图函数带了空格，在Lua里如何调用？

可以使用Lua原生的语言特性，通过 `[]` 来访问得到 `function` 对象后调用。

```lua
self["function name with space"]() -- “静态函数”
self["function name with space"](self) -- “实例函数”
```

## 为什么新建的工程安装了UnLua，打开后提示 `could not be compiled. Try rebuilding from source manually`？

新建工程时选择C++项目，而不是蓝图项目。

## 找不到UnLua的绑定按钮？

蓝图编辑器的窗口宽度太小，导致绑定图标被收缩到下拉按钮了，试试最大化编辑器窗口。

## 为什么覆写蓝图方法时，都需要带上 `Receive` 这样的前缀？

我们在蓝图中看到的方法名，很多都是经过对阅读友好化处理过的，以 `Actor` 上最常见的 `BeginPlay` 为例，在C++中是这样的：

```cpp
    /** Event when play begins for this actor. */
    UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "BeginPlay"))
	void ReceiveBeginPlay();
```

实际上蓝图中对应的就是 `ReceiveBeginPlay` 这个方法，只是通过 `DisplayName` 让我们在蓝图里看起来更友好一些。

## `UE.XXX` 好像访问不到我创建的蓝图类型？

从 `2.2.0` 版本开始，`UE.XXX` 只支持访问C++类型，蓝图类型需要先通过 `UE.UClass.Load` 加载再使用。

## 多继承层次里不能覆写接口的方法吗？

只能够覆写标记了 `BlueprintNativeEvent` 和 `BlueprintImplementationEvent` 的 `UFUNCTION`。

## 有没有办法只重写蓝图中的部分逻辑？

可以将部分逻辑合并成函数（Collapse To Function），然后在Lua中覆写这个函数。

## 如何使用调试？

参考文档：[调试](Debugging.md)

## 如何使用IntelliSense（智能提示）？

参考文档：[智能提示](IntelliSense.md)

## 怎么支持热更新Lua文件？

简单点可以利用UnLua默认的加载机制，会优先加载`FPaths::ProjectPersistentDownloadDir()`目录下的脚本。

以Windows平台举例，打包后工程里的Lua文件`require "A.B.C"`，会依次尝试加载：
1. WindowsNoEditor/项目名/Saved/PersistentDownloadDir/Content/Script/A/B/C.lua
2. WindowsNoEditor/项目名/Content/Script/A/B/C.lua

移动平台的下载目录则是：
- Android：/storage/emulated/0/Android/data/com.game.xxx/files/Content/Script/A/B.lua
- iOS：App的Document目录/PersistentDownloadDir/Content/Script/A/B.lua

也可以参考自定义加载器的示例来实现完全定制的加载策略。

## 为什么改了`package.path`没有效果，可以自定义`require`查找目录吗？

UE有自己的文件系统，如果有自定义查找目录的需求，可以修改`UnLua.PackagePath`来实现，比如：

```lua
UnLua.PackagePath = UnLua.PackagePath .. ';Plugins/UnLuaExtensions/LuaProtobuf/Content/Script/?.lua'
```

PS：默认值是`Content/Script/?.lua;Plugins/UnLua/Content/Script/?.lua`。

## 有哪些产品正在使用UnLua？

腾讯内部已知的有四十款左右项目在使用UnLua，外部项目暂时无法统计。
