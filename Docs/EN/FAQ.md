# FAQ

## Q: Overwritten RecieveTick but unuseful

A: Lua's Tick will only be called while Tick in Blueprint is enabled. This is designed by UE, you could enable Tick with two ways: one is set `bCanEverTick=true` in C++, and the other way is create `Event Tick` node in Blueprint, could see more details in `FKismetCompilerContext::SetCanEverTick()`.

## Q: Could not load Lua script files after packing

A: Add Lua script files path into `Additional Non-Asset Directories to Package` in UEEditor's Project Settings

## Q: Can't overwrite interface methods in multiple inheritance levels?

A: Only can override the `UFUNCTION`s with `BlueprintNativeEvent` property or `BlueprintImplementationEvent` property.

## Q: There is log of `Unknown bytecode 0xFF; ignoring it`

A: 0xFF is an Opcode register by UnLua, refer to [Implementation Principle](How_To_Implement_Overriding.md), it will not affect the use.

## Q: Is there a way to override only part of the logic in the blueprint

A: You can merge part of the logic into a function (Collapse To Function), and then overwrite this function in Lua.

## Q: How to use Lua debugging?

A: It is no different from ordinary lua process debugging, you can directly use the lua debugger to debug according to the general lua process debugging method.

## Q: How to use IntelliSense?

A: Set `ENABLE_INTELLISENSE=1` in `UnLuaIntelliSense.Build.cs` and regenerate the project, then you can generate Lua IntelliSense in the Intermediate directory under the UnLua plug-in, and add the IntelliSense to the IntelliSense directory of Lua IDE to realize UE smart prompts.

## Q: Which products are using UnLua?

A: There more than 20 projects using UnLua in Tencent, but unavailable to count projects out of Tencent.

## Q: Is there any info of G6IDE?

A: At present, G6IDE has been switched to VSCode plug-in development, and the new version will be released later.