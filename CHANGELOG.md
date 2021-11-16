# Change Log
All notable changes to this project will be documented in this file.
 
The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [2.0.1] - 2021-10-20

### Added

1. UMG释放引用教程
2. Lua热重载机制的开关
3. 部分英文文档

### Fixed

1. 首次切换场景到新场景的Actor的ReceiveBeginPlay没有调用到的问题 #268
2. 不同Actor绑定到同一个Lua脚本没有生效的问题 #269
3. 在编辑器中绑定某些对象时会报class is out of data的问题
4. 在Lua中获取对象属性异常时可能会导致Crash的问题
5. 在编辑器通过-game运行游戏时RestartLevel会导致Crash的问题

## [2.0.0] - 2021-09-18

1. 调整了UObject Ref 的释放机制
2. 更新并适配 Lua 5.4.2
3. 支持对非必要过滤对象的绑定过滤（包括load阶段产生的临时对象、抽象对象等）
4. 增加中文说明文档
5. 增加 Hotfix 功能（增加UnLuaFramework Module，并且使用时需要添加Content/Script 目录下的 G6HotfixHelper.lua/G6Hotfix.lua）
6. 修复若干bugs（参考commits）

## [1.2.0] - 2021-02-09

fix some bugs, could see details from commits

## [1.1.0] - 2020-04-04

Fix the crash bug caused incorrect userdata address.

## [1.0.0] - 2019-08-13

First version of UnLua.