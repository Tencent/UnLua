
除了UE默认在物理碰撞上提供的一些对象类型，我们也可以在 `项目设置 -> 引擎 -> 碰撞` 里新增项目自定义的碰撞配置。具体的配置操作可以参考[官方文档](https://docs.unrealengine.com/4.27/zh-CN/InteractiveExperiences/Physics/Collision/HowTo/AddCustomCollisionType/)。

在Lua中使用的时候，可以直接通过**配置名称**来访问，比如：

碰撞检测通道：
`UE.ECollisionChannel.Pawn`

物体类型查询：
`UE.EObjectTypeQuery.Player`

追踪类型查询:
`UE.ETraceTypeQuery.Weapon`
