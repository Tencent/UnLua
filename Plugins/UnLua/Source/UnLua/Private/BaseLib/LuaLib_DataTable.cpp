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

#include "UnLuaEx.h"
#include "LuaCore.h"
#include "Kismet/DataTableFunctionLibrary.h"

namespace UnLua
{
    /**
     * Get row data with structure.
     */
    static int32 UDataTable_GetRowDataStructure(lua_State* L)
    {
        int32 NumParams = lua_gettop(L);
        if (NumParams != 2)
            return luaL_error(L, "invalid parameters");

        UDataTable* Table = Cast<UDataTable>(UnLua::GetUObject(L, 1));
        if (!Table)
            return luaL_error(L, "invalid UDataTable");

        if (!IsType(L, 2, TType<FName>()))
            return luaL_error(L, "invalid row name");

        FName RowName = UnLua::Get(L, 2, TType<FName>());
        void* RowPtr = Table->FindRowUnchecked(RowName);

        if (RowPtr == nullptr)
        {
            lua_pushnil(L);
        }
        else
        {
            const UScriptStruct* StructType = Table->GetRowStruct();

            if (StructType != nullptr)
            {
                FString Name = FString("F" + StructType->GetName());
                uint8 StructPadding = StructType->GetMinAlignment();
                uint8 Padding = StructPadding < 8 ? 8 : StructPadding;
                void* Userdata = NewUserdataWithPadding(L, StructType->GetStructureSize(), TCHAR_TO_UTF8(*Name), Padding);
                if (Userdata != nullptr)
                {
                    if (StructType->StructFlags & STRUCT_CopyNative)
                    {
                        //Do ScriptStruct Construct
                        UScriptStruct::ICppStructOps* TheCppStructOps = StructType->GetCppStructOps();
                        TheCppStructOps->Construct(Userdata);
                    }
                    StructType->InitializeStruct(Userdata);
                    StructType->CopyScriptStruct(Userdata, RowPtr);
                }
            }
        }
        return 1;
    }

    static const luaL_Reg UDataTableLib[] =
    {
        {"GetRowDataStructure", UDataTable_GetRowDataStructure},
        {nullptr, nullptr}
    };

    BEGIN_EXPORT_REFLECTED_CLASS(UDataTableFunctionLibrary)
        ADD_STATIC_FUNCTION_EX("GetTableDataRowFromName", bool, Generic_GetDataTableRowFromName, const UDataTable*, FName, void*)
        ADD_STATIC_FUNCTION_EX("GetDataTableRowFromName", bool, Generic_GetDataTableRowFromName, const UDataTable*, FName, void*)
        ADD_LIB(UDataTableLib)
    END_EXPORT_CLASS()

    IMPLEMENT_EXPORTED_CLASS(UDataTableFunctionLibrary)
}
