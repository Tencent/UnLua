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

#include "LuaContainerInterface.h"

class FLuaArray
{
public:
    enum EScriptArrayFlag
    {
        OwnedByOther,   // 'ScriptArray' is owned by others
        OwnedBySelf,    // 'ScriptArray' is owned by self, it'll be freed in destructor
    };

    FLuaArray(const FScriptArray *InScriptArray, TSharedPtr<UnLua::ITypeInterface> InInnerInterface, EScriptArrayFlag Flag = OwnedByOther)
        : ScriptArray((FScriptArray*)InScriptArray), Inner(InInnerInterface), Interface(nullptr), ElementCache(nullptr), ElementSize(Inner->GetSize()), ScriptArrayFlag(Flag)
    {
        // allocate cache for a single element
        ElementCache = FMemory::Malloc(ElementSize, Inner->GetAlignment());
    }

    FLuaArray(const FScriptArray *InScriptArray, TLuaContainerInterface<FLuaArray> *InArrayInterface, EScriptArrayFlag Flag = OwnedByOther)
        : ScriptArray((FScriptArray*)InScriptArray), Interface(InArrayInterface), ElementCache(nullptr), ElementSize(0), ScriptArrayFlag(Flag)
    {
        if (Interface)
        {
            Inner = Interface->GetInnerInterface();
            ElementSize = Inner->GetSize();

            // allocate cache for a single element
            ElementCache = FMemory::Malloc(ElementSize, Inner->GetAlignment());
        }
    }

    ~FLuaArray()
    {
        DetachInterface();

        if (ScriptArrayFlag == OwnedBySelf)
        {
            delete ScriptArray;
        }
        FMemory::Free(ElementCache);
    }

    void DetachInterface()
    {
        if (Interface)
        {
            Interface->RemoveContainer(this);
            Interface = nullptr;
        }
    }

    FORCEINLINE void* GetContainerPtr() const { return ScriptArray; }

    /**
     * Check the validity of an index
     *
     * @param Index - the index
     * @return - true if the index is valid, false otherwise
     */
    FORCEINLINE bool IsValidIndex(int32 Index) const
    {
        return Index >= 0 && Index < Num();
    }

    /**
     * Get the length of the array
     *
     * @return - the length of the array
     */
    FORCEINLINE int32 Num() const
    {
        return ScriptArray->Num();
    }

    /**
     * Add an element to the array
     *
     * @param Item - the element
     * @return - the index of the added element
     */
    FORCEINLINE int32 Add(const void *Item)
    {
        const int32 Index = AddDefaulted();
        uint8 *Dest = GetData(Index);
        Inner->Copy(Dest, Item);
        return Index;
    }

    /**
     * Add a unique element to the array
     *
     * @param Item - the element
     * @return - the index of the added element
     */
    FORCEINLINE int32 AddUnique(const void *Item)
    {
        int32 Index = Find(Item);
        if (Index == INDEX_NONE)
        {
            Index = Add(Item);
        }
        return Index;
    }

    /**
     * Add N defaulted elements to the array
     *
     * @param Count - number of elements
     * @return - the index of the first element added
     */
    FORCEINLINE int32 AddDefaulted(int32 Count = 1)
    {
        int32 Index = ScriptArray->Add(Count, ElementSize);
        Construct(Index, Count);
        return Index;
    }

    /**
     * Add N uninitialized elements to the array
     *
     * @param Count - number of elements
     * @return - the index of the first element added
     */
    FORCEINLINE int32 AddUninitialized(int32 Count = 1)
    {
        return ScriptArray->Add(Count, ElementSize);
    }

    /**
     * Find an element
     *
     * @param Item - the element
     * @return - the index of the element
     */
    FORCEINLINE int32 Find(const void *Item) const
    {
        int32 Index = INDEX_NONE;
        for (int32 i = 0; i < Num(); ++i)
        {
            const uint8 *CurrentItem = GetData(i);
            if (Inner->Identical(Item, CurrentItem))
            {
                Index = i;
                break;
            }
        }
        return Index;
    }

    /**
     * Insert an element
     *
     * @param Item - the element
     * @param Index - the index
     */
    FORCEINLINE void Insert(const void *Item, int32 Index)
    {
        if (Index >= 0 && Index <= Num())
        {
            ScriptArray->Insert(Index, 1, ElementSize);
            Construct(Index, 1);
            uint8 *Dest = GetData(Index);
            Inner->Copy(Dest, Item);
        }
    }

    /**
     * Remove the i'th element
     *
     * @param Index - the index
     */
    FORCEINLINE void Remove(int32 Index)
    {
        if (IsValidIndex(Index))
        {
            Destruct(Index);
            ScriptArray->Remove(Index, 1, ElementSize);
        }
    }

    /**
     * Remove all elements equals to 'Item'
     *
     * @param Item - the element
     * @return - number of elements that be removed
     */
    FORCEINLINE int32 RemoveItem(const void *Item)
    {
        int32 NumRemoved = 0;
        int32 Index = Find(Item);
        while (Index != INDEX_NONE)
        {
            ++NumRemoved;
            Remove(Index);
            Index = Find(Item);
        }
        return NumRemoved;
    }

    /**
     * Empty the array
     */
    FORCEINLINE void Clear()
    {
        if (Num())
        {
            Destruct(0, Num());
            ScriptArray->Empty(0, ElementSize);
        }
    }

    /**
     * Reserve space for N elements
     *
     * @param Size - the element
     * @return - whether the operation succeed
     */
    FORCEINLINE bool Reserve(int32 Size)
    {
        if (Num() > 0)
        {
            return false;
        }
        ScriptArray->Empty(Size, ElementSize);
        return true;
    }

    /**
     * Resize the array to new size
     *
     * @param NewSize - new size of the array
     */
    FORCEINLINE void Resize(int32 NewSize)
    {
        if (NewSize >= 0)
        {
            int32 Count = NewSize - Num();
            if (Count > 0)
            {
                AddDefaulted(Count);
            }
            else if (Count < 0)
            {
                Destruct(NewSize, -Count);
                ScriptArray->Remove(NewSize, -Count, ElementSize);
            }
        }
    }

    /**
     * Get value of the i'th element
     *
     * @param Index - the index
     * @param OutItem - the element in the 'Index'
     */
    FORCEINLINE void Get(int32 Index, void *OutItem) const
    {
        if (IsValidIndex(Index))
        {
            Inner->Copy(OutItem, GetData(Index));
        }
    }

    /**
     * Set new value for the i'th element
     *
     * @param Index - the index
     * @param Item - the element to be set
     */
    FORCEINLINE void Set(int32 Index, const void *Item)
    {
        if (IsValidIndex(Index))
        {
            Inner->Copy(GetData(Index), Item);
        }
    }

    /**
     * Swap two elements
     *
     * @param A - the first index
     * @param B - the second index
     */
    FORCEINLINE void Swap(int32 A, int32 B)
    {
        if (A != B)
        {
            if (IsValidIndex(A) && IsValidIndex(B))
            {
                ScriptArray->SwapMemory(A, B, ElementSize);
            }
        }
    }

    /**
     * Shuffle the elements
     */
    FORCEINLINE void Shuffle()
    {
        int32 LastIndex = Num() - 1;
        for (int32 i = 0; i <= LastIndex; ++i)
        {
            int32 Index = FMath::RandRange(i, LastIndex);
            if (i != Index)
            {
                ScriptArray->SwapMemory(i, Index, ElementSize);
            }
        }
    }

    /**
     * Append another array
     *
     * @param SourceArray - the array to be appended
     */
    FORCEINLINE void Append(const FLuaArray &SourceArray)
    {
        if (SourceArray.Num() > 0)
        {
            int32 Index = AddDefaulted(SourceArray.Num());
            for (int32 i = 0; i < SourceArray.Num(); ++i)
            {
                uint8 *Dest = GetData(Index++);
                const uint8 *Src = SourceArray.GetData(i);
                Inner->Copy(Dest, Src);
            }
        }
    }

    /**
     * Get address of the i'th element
     *
     * @param Index - the index
     * @return - the address of the i'th element
     */
    FORCEINLINE uint8* GetData(int32 Index)
    {
        return (uint8*)ScriptArray->GetData() + Index * ElementSize;
    }

    FORCEINLINE const uint8* GetData(int32 Index) const
    {
        return (uint8*)ScriptArray->GetData() + Index * ElementSize;
    }

    /**
     * Get the address of the allocated memory
     *
     * @return - the address of the allocated memory
     */
    FORCEINLINE void* GetData()
    {
        return ScriptArray->GetData();
    }

    FORCEINLINE const void* GetData() const
    {
        return ScriptArray->GetData();
    }

    FScriptArray *ScriptArray;
    TSharedPtr<UnLua::ITypeInterface> Inner;
    TLuaContainerInterface<FLuaArray> *Interface;
    void *ElementCache;            // can only hold one element...
    int32 ElementSize;
    EScriptArrayFlag ScriptArrayFlag;

private:
    /**
     * Construct n elements
     */
    FORCEINLINE void Construct(int32 Index, int32 Count = 1)
    {
        uint8 *Dest = GetData(Index);
        for (int32 i = 0; i < Count; ++i)
        {
            Inner->Initialize(Dest);
            Dest += ElementSize;
        }
    }

    /**
     * Destruct n elements
     */
    FORCEINLINE void Destruct(int32 Index, int32 Count = 1)
    {
        uint8 *Dest = GetData(Index);
        for (int32 i = 0; i < Count; ++i)
        {
            Inner->Destruct(Dest);
            Dest += ElementSize;
        }
    }
};
