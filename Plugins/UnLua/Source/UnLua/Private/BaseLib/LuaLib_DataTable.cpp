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
#include "Engine/DataTable.h"
#include "Kismet/DataTableFunctionLibrary.h"
#include "ReflectionUtils/PropertyCreator.h"

namespace UnLua
{
	/**
	 * Get row data with structure.
	 */
	static int32 UDataTable_GetRowDataStructure(lua_State* L)
	{
		int32 NumParams = lua_gettop(L);
		if (NumParams != 2)
		{
			UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
			return 0;
		}

		UDataTable* Table = Cast<UDataTable>(UnLua::GetUObject(L, 1));
		if (!Table)
		{
			UE_LOG(LogUnLua, Log, TEXT("%s: Invalid Table!"), ANSI_TO_TCHAR(__FUNCTION__));
			return 0;
		}

		if (!IsType(L, 2, TType<FName>()))
		{
			UE_LOG(LogUnLua, Log, TEXT("%s: Invalid Name!"), ANSI_TO_TCHAR(__FUNCTION__));
			return 0;
		}
		FName RowName = UnLua::Get(L, 2, TType<FName>());

		void* RowPtr = Table->FindRowUnchecked(RowName);
		if (RowPtr == nullptr)
		{
			lua_pushnil(L);
		}
		else
		{
			const UScriptStruct* StructType = Table->GetRowStruct();

			if (StructType != nullptr) {
				FString Name = FString("F" + StructType->GetName());
				uint8 StructPadding = StructType->GetMinAlignment();
				uint8 Padding = StructPadding < 8 ? 8 : StructPadding;
				void* Userdata = NewUserdataWithPadding(L, StructType->GetStructureSize(), TCHAR_TO_UTF8(*Name), Padding);
				if (Userdata != nullptr) {
					StructType->InitializeStruct(Userdata);
					if (StructType->StructFlags & STRUCT_CopyNative) {
						//Do ScriptStruct Construct
						UScriptStruct::ICppStructOps* TheCppStructOps = StructType->GetCppStructOps();
						TheCppStructOps->Construct(Userdata);
					}
					StructType->CopyScriptStruct(Userdata, RowPtr);
					return 1;
				}
			}
		}

		lua_pushnil(L);
		return 1;
	}

	static const luaL_Reg UDataTableFunctionLibraryLib[] =
	{
		{ "GetRowDataStructure", UDataTable_GetRowDataStructure },
		{ nullptr, nullptr }
	};


	//export GetTableDataRowFromName
	BEGIN_EXPORT_REFLECTED_CLASS(UDataTableFunctionLibrary)
	ADD_STATIC_FUNCTION_EX("GetTableDataRowFromName", bool, Generic_GetDataTableRowFromName, const UDataTable*, FName, void*)
	ADD_STATIC_FUNCTION_EX("GetDataTableRowFromName", bool, Generic_GetDataTableRowFromName, const UDataTable*, FName, void*)
	ADD_LIB(UDataTableFunctionLibraryLib)
	END_EXPORT_CLASS()
	IMPLEMENT_EXPORTED_CLASS(UDataTableFunctionLibrary)
}

static int32 UDataTable_ToArray(lua_State* L)
{
	int32 NumParams = lua_gettop(L);
	if (NumParams != 1)
	{
		UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	UDataTable* DataTable = Cast<UDataTable>(UnLua::GetUObject(L, 1));
	if (!DataTable)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid source DataTable!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	TSharedPtr<UnLua::ITypeInterface> TypeInterface(GPropertyCreator.CreateStructProperty(DataTable->RowStruct));
	if (!TypeInterface)
	{
		UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Failed to create TArray!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	FScriptArray* ScriptArray = new FScriptArray;
	void* Userdata = NewScriptContainer(L, FScriptContainerDesc::Array);
	new(Userdata) FLuaArray(ScriptArray, TypeInterface, FLuaArray::OwnedBySelf);

	FLuaArray* Array = (FLuaArray*)Userdata;
	for (TPair<FName, uint8*> Pair : DataTable->GetRowMap())
	{
		int32 Index = Array->AddDefaulted();
		uint8* Data = Array->GetData(Index);
		Array->Inner->Copy(Data, Pair.Value);
	}

	return 1;
}

static const luaL_Reg UDataTableLib[] =
{
	{ "ToArray", UDataTable_ToArray },
	{ nullptr, nullptr }
};

BEGIN_EXPORT_REFLECTED_CLASS(UDataTable)
ADD_LIB(UDataTableLib)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(UDataTable)