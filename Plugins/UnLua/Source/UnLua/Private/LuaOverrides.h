// Tencent is pleased to support the open source community by making UnLua available.
// 
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the MIT License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.


#pragma once

#include "LuaOverridesClass.h"

namespace UnLua
{
    class FLuaOverrides : public FUObjectArray::FUObjectDeleteListener
    {
    public:
        static FLuaOverrides& Get();

        virtual void NotifyUObjectDeleted(const UObjectBase* Object, int32 Index) override;

        virtual void OnUObjectArrayShutdown() override;

        /** 在指定Class上新增或覆盖一个函数，调用时转发到Lua */
        void Override(UFunction* Function, UClass* Class, FName NewName);

        /**
         * 还原指定UClass上所有覆写的UFunction。
         * 在退出PIE时使用。
         * @return 返回一个临时UClass，其中包含了所有被抽出来的ULuaFunction
         */
        void Restore(UClass* Class);

        /** 还原所有已覆写的UClass */
        void RestoreAll();

    private:
        /**
         * 获取指定Class所对应的LuaOverridesClass，用于承载所有运行时创建的ULuaFunction
         */
        UClass* GetOrAddOverridesClass(UClass* Class);

        TMap<UClass*, ULuaOverridesClass*> Overrides;
    };
}
