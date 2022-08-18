# 插件与模块说明

## UnLua

核心功能插件，安装使用UnLua时至少要保证你的工程Plugin目录下有它。

模块列表：
* UnLua 主要运行时模块
* UnLuaEditor 编辑器模块，提供Lua模版生成/智能提示导出/控制台命令等功能
* UnLuaDefaultParamCollector 编码模块，收集UFUNCTION的默认参数

## UnLuaExtensions

UnLua扩展示例插件，在需要集成一些第三方库时可以参考它的实现。

模块列表：
* [LuaSocket](https://github.com/lunarmodules/luasocket) 一些纯Lua调试器可能会需要这个库

## UnLuaTestSuite

自动化测试插件，覆盖了UnLua提供的API的规范测试以及一些Issue对应的回归测试。