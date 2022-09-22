# 功能清单

## 概览
* 静态绑定和动态绑定
* 编辑器环境下模拟服务端/客户端
* 蓝图模版生成与导出
* 手动静态导出
* 结构体访问优化
* 通过 `UE` 命名空间访问C++类型

## 平台支持
* 运行平台：Windows / Android / iOS / Linux / OSX
* 引擎版本：Unreal Engine 4.17.x - Unreal Engine 5.x

## 支持使用Lua替换实现
* 蓝图里所有标记了 `BlueprintImplementableEvent` 或 `BlueprintNativeEvent` 的"蓝图事件"
* 所有网络复制事件通知（Replication Notify）
* 所有动画事件通知（Animation Notify）
* 所有输入事件通知（Input Event）

## Lua调用蓝图
* 支持访问所有 `UCLASS`, `UPROPERTY`, `UFUNCTION`, `USTRUCT`, `UENUM`
* 支持访问已被覆盖掉的原蓝图逻辑
* 支持所有标记了 `BlueprintCallable` 或 `Exec` 的 `UFUNCTION` 的默认参数
* 支持在协程里调用 `Latent` 函数

## Lua调用C++
* 单播委托绑定、解绑、触发
* 多播委托绑定、解绑、清理、广播
* 访问 `UENUM`
* 支持[自定义碰撞枚举](CollisionEnum.md)
* 成员变量
* 成员函数
* 静态成员变量
* 静态成员函数
* 全局函数

## C++调用Lua
* 全局函数
* 全局表里的函数

## 其他工具
* 支持[自定义生成Lua模版](CustomTemplate.md)
