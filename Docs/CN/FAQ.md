# FAQ 常见问题解答

## Q：覆写RecieveTick无效？(Overwritten RecieveTick but unuseful)

A：只有当蓝图中的Tick事件启用时，Lua中的Tick才会执行。这个是UE自己的设计，开启Tick有两种方法，一种是在C++中把bCanEverTick设为true，另一种就是在蓝图中创建Event Tick节点，可见FKismetCompilerContext::SetCanEverTick()函数。
A: Lua's Tick will only be called while Tick in Blueprint is enabled. This is designed by UE, you could enable Tick with two ways: one is set `bCanEverTick=true` in C++, and the other way is create `Event Tick` node in Blueprint, could see more details in `FKismetCompilerContext::SetCanEverTick()`.

## Q：Lua代码打包后加载不到？(Could not load Lua script files after packing)

A：在 "Additional Non-Asset Directories to Package" 一栏中加入Script目录。
A: Add Lua script files path into `Additional Non-Asset Directories to Package` in UEEditor's Project Settings

## Q：多继承层次里不能覆写接口的方法吗？(Can't overwrite interface methods in multiple inheritance levels?)

A：只能够覆写标记了 `BlueprintNativeEvent` 和 `BlueprintImplementationEvent` 的 `UFUNCTION`。
A: Only can override the `UFUNCTION`s with `BlueprintNativeEvent` property or `BlueprintImplementationEvent` property.

## Q：出现 “Unknown bytecode 0xFF; ignoring it” 日志？(There is log of `Unknown bytecode 0xFF; ignoring it`)

A：0xFF是插件新注册的一个Opcode，参考[实现原理](How_To_Implement_Overriding.md)，不会影响使用。
A: 0xFF is an Opcode register by UnLua, refer to [Implementation Principle](How_To_Implement_Overriding.md), it will not affect the use.

## Q：有没有办法只重写蓝图中的部分逻辑？(Is there a way to override only part of the logic in the blueprint)

A：可以将部分逻辑合并成函数（Collapse To Function），然后在Lua中覆写这个函数。
A: You can merge part of the logic into a function (Collapse To Function), and then overwrite this function in Lua.

## Q：如何使用调试？(How to use Lua debugging?)

A：和普通lua进程调试无区别，可以直接使用lua调试器按照一般lua进程的调试方式进行调试
A: It is no different from ordinary lua process debugging, you can directly use the lua debugger to debug according to the general lua process debugging method.

## Q：如何使用IntelliSense（智能提示）？(How to use IntelliSense?)

A：设置 UnLuaIntelliSense.Build.cs 中 ENABLE_INTELLISENSE=1，并重新生成工程，则可以在UnLua 插件下的Intermediate目录中生成Lua IntelliSense，将该IntelliSense添加到Lua IDE的IntelliSense目录下，则可以实现UE的智能提示。
A: Set `ENABLE_INTELLISENSE=1` in `UnLuaIntelliSense.Build.cs` and regenerate the project, then you can generate Lua IntelliSense in the Intermediate directory under the UnLua plug-in, and add the IntelliSense to the IntelliSense directory of Lua IDE to realize UE smart prompts.

## Q：有哪些产品正在使用UnLua？(Which products are using UnLua?)

A：腾讯内部已知的有二十款左右项目在使用UnLua，外部项目暂时无法统计。
A: There more than 20 projects using UnLua in Tencent, but unavailable to count projects out of Tencent.

## Q：G6IDE的相关消息？(Is there any info of G6IDE?)

A：目前G6IDE已经切换成 VSCode插件开发，新版本后面也会对外发布
A: At present, G6IDE has been switched to VSCode plug-in development, and the new version will be released later.