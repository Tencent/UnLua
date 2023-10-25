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

#include "LuaArray.h"
#include "Runtime/Launch/Resources/Version.h"

class FLuaMap
{
public:
    struct FLuaMapEnumerator
    {
        FLuaMapEnumerator(FLuaMap* InLuaMap, const int32 InIndex): LuaMap(InLuaMap), Index(InIndex)
        {
        }

        static int gc(lua_State* L)
        {
            if (FLuaMapEnumerator** Enumerator = (FLuaMapEnumerator**)lua_touserdata(L, 1))
            {
                (*Enumerator)->LuaMap = nullptr;

                delete *Enumerator;
            }

            return 0;
        }

        FLuaMap* LuaMap;

        int32 Index = 0;
    };
    
    enum FScriptMapFlag
    {
        OwnedByOther,   // 'Map' is owned by others
        OwnedBySelf,    // 'Map' is owned by self, it'll be freed in destructor
    };

    FLuaMap(const FScriptMap *InScriptMap, TSharedPtr<UnLua::ITypeInterface> InKeyInterface, TSharedPtr<UnLua::ITypeInterface> InValueInterface, FScriptMapFlag Flag = OwnedByOther)
        : Map((FScriptMap*)InScriptMap), MapLayout(FScriptMap::GetScriptLayout(InKeyInterface->GetSize(), InKeyInterface->GetAlignment(), InValueInterface->GetSize(), InValueInterface->GetAlignment()))
        , KeyInterface(InKeyInterface), ValueInterface(InValueInterface), Interface(nullptr), ElementCache(nullptr), ScriptMapFlag(Flag)
    {
        FStructBuilder StructBuilder;
        StructBuilder.AddMember(InKeyInterface->GetSize(), InKeyInterface->GetAlignment());
        StructBuilder.AddMember(InValueInterface->GetSize(), InValueInterface->GetAlignment());
        // allocate cache for a key-value pair with alignment
        ElementCache = FMemory::Malloc(StructBuilder.GetSize(), StructBuilder.GetAlignment());
        UNLUA_STAT_MEMORY_ALLOC(ElementCache, ContainerElementCache);
    }

    FLuaMap(const FScriptMap *InScriptMap, TLuaContainerInterface<FLuaMap> *InMapInterface, FScriptMapFlag Flag = OwnedByOther)
        : Map((FScriptMap*)InScriptMap), Interface(InMapInterface), ElementCache(nullptr), ScriptMapFlag(Flag)
    {
        if (Interface)
        {
            KeyInterface = Interface->GetInnerInterface();
            ValueInterface = Interface->GetExtraInterface();
            MapLayout = FScriptMap::GetScriptLayout(KeyInterface->GetSize(), KeyInterface->GetAlignment(), ValueInterface->GetSize(), ValueInterface->GetAlignment());

            FStructBuilder StructBuilder;
            StructBuilder.AddMember(KeyInterface->GetSize(), KeyInterface->GetAlignment());
            StructBuilder.AddMember(ValueInterface->GetSize(), ValueInterface->GetAlignment());
            // allocate cache for a key-value pair with alignment
            ElementCache = FMemory::Malloc(StructBuilder.GetSize(), StructBuilder.GetAlignment());
            UNLUA_STAT_MEMORY_ALLOC(ElementCache, ContainerElementCache);
        }
    }

    ~FLuaMap()
    {
        if (ScriptMapFlag == OwnedBySelf)
        {
            Clear();
            delete Map;
        }
        UNLUA_STAT_MEMORY_FREE(ElementCache, ContainerElementCache);
        FMemory::Free(ElementCache);
    }

    FORCEINLINE FScriptMap* GetContainerPtr() const { return Map; }

    /**
     * Get the length of the map
     *
     * @return - the length of the array
     */
    FORCEINLINE int32 Num() const
    {
        //return MapHelper.Num();
        return Map->Num();
    }

    /**
     * Get the max index of the map
     *
     * @return - the max index of the array
     */
    FORCEINLINE int32 GetMaxIndex() const
    {
        //return MapHelper.GetMaxIndex();
        return Map->GetMaxIndex();
    }

    /**
     * Add a pair to the map
     *
     * @param Key - the key
     * @param Value - the value
     */
    FORCEINLINE void Add(const void *Key, const void *Value)
    {
        //MapHelper.AddPair(Key, Value);
        const UnLua::ITypeInterface *LocalKeyInterface = KeyInterface.Get();
        const UnLua::ITypeInterface *LocalValueInterface = ValueInterface.Get();
        Map->Add(Key, Value, MapLayout,
            [LocalKeyInterface](const void* ElementKey) { return LocalKeyInterface->GetValueTypeHash(ElementKey); },
            [LocalKeyInterface](const void* A, const void* B) { return LocalKeyInterface->Identical(A, B); },
            [LocalKeyInterface, Key](void* NewElementKey)
            {
                LocalKeyInterface->Initialize(NewElementKey);
                LocalKeyInterface->Copy(NewElementKey, Key);
            },
            [LocalValueInterface, Value](void* NewElementValue)
            {
                LocalValueInterface->Initialize(NewElementValue);
                LocalValueInterface->Copy(NewElementValue, Value);
            },
            [LocalValueInterface, Value](void* ExistingElementValue)
            {
                LocalValueInterface->Copy(ExistingElementValue, Value);
            },
            [LocalKeyInterface](void* ElementKey)
            {
                if (!LocalKeyInterface->IsPODType() && !LocalKeyInterface->IsTriviallyDestructible())
                {
                    LocalKeyInterface->Destruct(ElementKey);
                }
            },
            [LocalValueInterface](void* ElementValue)
            {
                if (!LocalValueInterface->IsPODType() && !LocalValueInterface->IsTriviallyDestructible())
                {
                    LocalValueInterface->Destruct(ElementValue);
                }
            }
        );
    }

    /**
     * Remove a pair from the map
     *
     * @param Key - the key
     */
    FORCEINLINE bool Remove(const void *Key)
    {
        //return MapHelper.RemovePair(Key);
        const UnLua::ITypeInterface *LocalKeyInterface = KeyInterface.Get();
        if (uint8* Entry = Map->FindValue(Key, MapLayout,
            [LocalKeyInterface](const void* ElementKey) { return LocalKeyInterface->GetValueTypeHash(ElementKey); },
            [LocalKeyInterface](const void* A, const void* B) { return LocalKeyInterface->Identical(A, B); }
        ))
        {
            int32 Idx = (Entry - (uint8*)Map->GetData(0, MapLayout)) / MapLayout.SetLayout.Size;
            DestructItems(Idx, 1);
            Map->RemoveAt(Idx, MapLayout);
            return true;
        }
        return false;
    }

    /**
     * Find the associated value of a Key from the map
     *
     * @param Key - the key
     * @param OutValue - the result value
     * @return - true if the key is found, false otherwise
     */
    FORCEINLINE bool Find(const void *Key, void *OutValue)
    {
        //uint8 *Value = MapHelper.FindValueFromHash(Key);
        //if (Value)
        //{
        //    check(OutValue);
        //    //MapHelper.ValueProp->InitializeValue(OutValue);
        //    MapHelper.ValueProp->CopyCompleteValue(OutValue, Value);
        //    return true;
        //}
        const UnLua::ITypeInterface *LocalKeyInterface = KeyInterface.Get();
        uint8 *Value = Map->FindValue(Key, MapLayout,
            [LocalKeyInterface](const void* ElementKey) { return LocalKeyInterface->GetValueTypeHash(ElementKey); },
            [LocalKeyInterface](const void* A, const void* B) { return LocalKeyInterface->Identical(A, B); }
        );
        if (Value)
        {
            check(OutValue);
            ValueInterface->Copy(OutValue, Value);
            return true;
        }
        return false;
    }

    /**
     * Find the associated value of a Key from the map
     *
     * @param Key - the key
     * @return - the address of the associated value
     */
    FORCEINLINE void* Find(const void *Key)
    {
        //return MapHelper.FindValueFromHash(Key);
        const UnLua::ITypeInterface *LocalKeyInterface = KeyInterface.Get();
        return Map->FindValue(Key, MapLayout,
            [LocalKeyInterface](const void* ElementKey) { return LocalKeyInterface->GetValueTypeHash(ElementKey); },
            [LocalKeyInterface](const void* A, const void* B) { return LocalKeyInterface->Identical(A, B); }
        );
    }

    /**
     * Empty the map, and reallocate it for the expected number of pairs.
     *
     * @param Slack (Optional) - the expected usage size after empty operation. Default is 0.
     */
    FORCEINLINE void Clear(int32 Slack = 0)
    {
        //MapHelper.EmptyValues(Slack);
        int32 OldNum = Num();
        if (OldNum)
        {
            DestructItems(0, OldNum);
        }
        if (OldNum || Slack)
        {
            Map->Empty(Slack, MapLayout);
        }
    }

    /**
     * Get address of the i'th pair
     *
     * @param Index - the index
     * @return - the address of the i'th pair
     */
    FORCEINLINE uint8* GetData(int32 Index)
    {
        return (uint8*)Map->GetData(Index, MapLayout);
    }

    FORCEINLINE const uint8* GetData(int32 Index) const
    {
        return (uint8*)Map->GetData(Index, MapLayout);
    }

    /**
     * Adds an uninitialized pair to the map. The map needs rehashing to make it valid.
     *
     * @return - the index of the added pair.
     */
    FORCEINLINE int32 AddUninitializedValue()
    {
        checkSlow(Num() >= 0);
        return Map->AddUninitialized(MapLayout);
    }

    /**
     * Adds a blank, constructed pair to the map. The map needs rehashing to make it valid.
     *
     * @return - the index of the first pair added.
     **/
    FORCEINLINE int32 AddDefaultValue_Invalid_NeedsRehash()
    {
        checkSlow(Num() >= 0);
        int32 Result = AddUninitializedValue();
        ConstructItem(Result);
        return Result;
    }

    /**
     * Rehash the pairs in the map. this function must be called to create a valid map.
     */
    FORCEINLINE void Rehash()
    {
        Map->Rehash(MapLayout, [this](const void* Src) { return KeyInterface->GetValueTypeHash(Src); });
    }

    /**
     * Convert the keys to an array
     *
     * @param OutArray - the memory the result array resides
     * @return - the result array
     */
    FORCEINLINE FLuaArray* Keys(void *OutArray) const
    {
        if (KeyInterface && OutArray)
        {
            FScriptArray *ScriptArray = new FScriptArray;
            //ArrayProperty->InitializeValue(ScriptArray);        // do nothing...
            FLuaArray *LuaArray = new(OutArray) FLuaArray(ScriptArray, KeyInterface, FLuaArray::OwnedBySelf);
            int32 i = -1, Size = Map->Num();
            while (Size > 0)
            {
                if (IsValidIndex(++i))
                {
                    int32 KeyOffset = 0;
#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION < 22
                    KeyOffset = MapLayout.KeyOffset;
#endif
                    LuaArray->Add((uint8*)Map->GetData(i, MapLayout) + KeyOffset);
                    --Size;
                }
            }
            return LuaArray;
        }
        return nullptr;
    }

    /**
     * Convert the values to an array
     *
     * @param OutArray - the memory the result array resides
     * @return - the result array
     */
    FORCEINLINE_DEBUGGABLE FLuaArray* Values(void *OutArray) const
    {
        if (ValueInterface && OutArray)
        {
            FScriptArray *ScriptArray = new FScriptArray;
            //ArrayProperty->InitializeValue(ScriptArray);        // do nothing...
            FLuaArray *LuaArray = new(OutArray) FLuaArray(ScriptArray, ValueInterface, FLuaArray::OwnedBySelf);
            int32 i = -1, Size = Map->Num();
            while (Size > 0)
            {
                if (IsValidIndex(++i))
                {
                    LuaArray->Add((uint8*)Map->GetData(i, MapLayout) + MapLayout.ValueOffset);
                    --Size;
                }
            }
            return LuaArray;
        }
        return nullptr;
    }

    FORCEINLINE bool IsValidIndex(int32 Index) const
    {
        return Map->IsValidIndex(Index);
    }

    FScriptMap *Map;
    FScriptMapLayout MapLayout;
    TSharedPtr<UnLua::ITypeInterface> KeyInterface;
    TSharedPtr<UnLua::ITypeInterface> ValueInterface;
    TLuaContainerInterface<FLuaMap> *Interface;
    //FScriptMapHelper MapHelper;
    void *ElementCache;             // can only hold a key-value pair
    FScriptMapFlag ScriptMapFlag;

private:
    void DestructItems(int32 Index, int32 Count)
    {
        check(Index >= 0 && Count >= 0);

        if (Count == 0)
        {
            return;
        }

        bool bDestroyKeys = !KeyInterface->IsPODType() && !KeyInterface->IsTriviallyDestructible();
        bool bDestroyValues = !ValueInterface->IsPODType() && !ValueInterface->IsTriviallyDestructible();
        if (bDestroyKeys || bDestroyValues)
        {
            uint32 Stride = MapLayout.SetLayout.Size;
            uint8* PairPtr = (uint8*)Map->GetData(Index, MapLayout);
            if (bDestroyKeys)
            {
                if (bDestroyValues)
                {
                    for (; Count; ++Index)
                    {
                        if (IsValidIndex(Index))
                        {
                            KeyInterface->Destruct(PairPtr);
                            ValueInterface->Destruct(PairPtr + MapLayout.ValueOffset);
                            --Count;
                        }
                        PairPtr += Stride;
                    }
                }
                else
                {
                    for (; Count; ++Index)
                    {
                        if (IsValidIndex(Index))
                        {
                            KeyInterface->Destruct(PairPtr);
                            --Count;
                        }
                        PairPtr += Stride;
                    }
                }
            }
            else
            {
                for (; Count; ++Index)
                {
                    if (IsValidIndex(Index))
                    {
                        ValueInterface->Destruct(PairPtr + MapLayout.ValueOffset);
                        --Count;
                    }
                    PairPtr += Stride;
                }
            }
        }
    }

    FORCEINLINE void ConstructItem(int32 Index)
    {
        check(IsValidIndex(Index));
        uint8* Dest = (uint8*)Map->GetData(Index, MapLayout);
        void *ValueDest = Dest + MapLayout.ValueOffset;
        KeyInterface->Initialize(Dest);
        ValueInterface->Initialize(ValueDest);
    }
};
