# 调试
## 使用 LuaPanda / LuaHelper 调试

### 一、准备工作
1. 从VSCode应用市场安装 [LuaPanda](https://marketplace.visualstudio.com/items?itemName=stuartwang.luapanda) / [LuaHelper](https://marketplace.visualstudio.com/items?itemName=yinfei.luahelper)
2. 从 [LuaPanda官方仓库](https://github.com/Tencent/LuaPanda/tree/master/Debugger) 获取`LuaPanda.lua`，放入`{UE工程}/Content/Script`目录
3. 在Lua代码中加入 `require("LuaPanda").start("127.0.0.1",8818)`

### 二、开始调试
VSCode环境请参考各插件的配置文档，这里不再赘述。
- [LuaPanda](https://github.com/Tencent/LuaPanda)
- [LuaHelper](https://github.com/Tencent/LuaHelper)

注：调试器依赖`luasocket`，UnLua已通过扩展插件集成，如果发现无法连接请检查`{UE工程}/Plugins/UnLuaExtensions`目录是否存在。

## 使用 LuaBooster 调试

TODO: