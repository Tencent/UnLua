# FAQ 常见问题解答

## Q：覆写RecieveTick无效？

A：只有当蓝图中的Tick事件启用时，Lua中的Tick才会执行。这个是UE4自己的设计，开启Tick有两种方法，一种是在C++中把bCanEverTick设为true，另一种就是在蓝图中创建Event Tick节点，可见FKismetCompilerContext::SetCanEverTick()函数。

## Q：Lua代码打包后加载不到？

A：在 "Additional Non-Asset Directories to Package" 一栏中加入Script目录。

## Q：多继承层次里不能覆写接口的方法吗？

A：只能够覆写标记了 `BlueprintNativeEvent` 和 `BlueprintImplementationEvent` 的 `UFUNCTION`。

## Q：出现 “Unknown bytecode 0xFF; ignoring it” 日志？

A：0xFF是插件新注册的一个Opcode，参考[实现原理](How_To_Implement_Overriding.md)，不会影响使用。

## Q：有没有办法只重写蓝图中的部分逻辑？

A：可以将部分逻辑合并成函数（Collapse To Function），然后在Lua中覆写这个函数。

## Q：如何使用调试？

A：和普通lua进程调试无区别，可以直接使用lua调试器按照一般lua进程的调试方式进行调试

## Q：如何使用IntelliSense（智能提示）？

A：设置 UnLuaIntelliSense.Build.cs 中 ENABLE_INTELLISENSE=1，并重新生成工程，则可以在UnLua 插件下的Intermediate目录中生成Lua IntelliSense，将该IntelliSense添加到Lua IDE的IntelliSense目录下，则可以实现UE4的智能提示。

## Q：有哪些产品正在使用UnLua？

A：腾讯内部已知的有二十款左右项目在使用UnLua，外部项目暂时无法统计。

## Q：G6IDE的相关消息？

A：目前G6IDE已经切换成 VSCode插件开发，新版本后面也会对外发布