#include "LuaOverrides.h"
#include "LuaFunction.h"
#include "LuaOverridesClass.h"
#include "UObject/MetaData.h"

namespace UnLua
{
    FLuaOverrides& FLuaOverrides::Get()
    {
        static FLuaOverrides Override;
        return Override;
    }

    FLuaOverrides::FLuaOverrides()
    {
        GUObjectArray.AddUObjectDeleteListener(this);
    }

    void FLuaOverrides::NotifyUObjectDeleted(const UObjectBase* Object, int32 Index)
    {
        TWeakObjectPtr<ULuaOverridesClass> OverridesClass;
        if (Overrides.RemoveAndCopyValue((UClass*)Object, OverridesClass) )
        {
            if ( OverridesClass.IsValid( ) )
            {
                OverridesClass->Restore();
             }
         }
    }

    void FLuaOverrides::OnUObjectArrayShutdown()
    {
        GUObjectArray.RemoveUObjectDeleteListener(this);
    }

    void FLuaOverrides::Override(UFunction* Function, UClass* Class, FName NewName)
    {
        const auto OverridesClass = GetOrAddOverridesClass(Class);

        ULuaFunction* LuaFunction;
        const auto bAddNew = Function->GetOuter() != Class;
        if (bAddNew)
        {
            const auto Exists = Class->FindFunctionByName(NewName, EIncludeSuperFlag::ExcludeSuper);
            if (Exists && Exists->GetSuperStruct() == Function)
                return;
        }
        else
        {
            LuaFunction = Cast<ULuaFunction>(Function);
            if (LuaFunction)
            {
                LuaFunction->Initialize();
                return;
            }
        }

        const auto OriginalFunctionFlags = Function->FunctionFlags;
        Function->FunctionFlags &= (~EFunctionFlags::FUNC_Native);

        FObjectDuplicationParameters DuplicationParams(Function, OverridesClass);
        DuplicationParams.InternalFlagMask &= ~EInternalObjectFlags::Native;
        DuplicationParams.DestName = NewName;
        DuplicationParams.DestClass = ULuaFunction::StaticClass();
        LuaFunction = static_cast<ULuaFunction*>(StaticDuplicateObjectEx(DuplicationParams));

        Function->FunctionFlags = OriginalFunctionFlags;
        LuaFunction->FunctionFlags = OriginalFunctionFlags;

        LuaFunction->Next = OverridesClass->Children;
        OverridesClass->Children = LuaFunction;

        LuaFunction->StaticLink(true);
        LuaFunction->Initialize();
        LuaFunction->Override(Function, Class, bAddNew);
        LuaFunction->Bind();

        if (Class->IsRooted() || GUObjectArray.IsDisregardForGC(Class))
            LuaFunction->AddToRoot();
        else
            LuaFunction->AddToCluster(Class);
    }

    void FLuaOverrides::Restore(UClass* Class)
    {
        TWeakObjectPtr<ULuaOverridesClass> OverridesClass;
        if ( !Overrides.RemoveAndCopyValue( Class, OverridesClass) )
            return;
            
        if ( !OverridesClass.IsValid() )
            return;

        OverridesClass->Restore();
    }

    void FLuaOverrides::RestoreAll()
    {
        TArray<UClass*> Classes;
        Overrides.GenerateKeyArray(Classes);
        for (const auto& Class : Classes)
            Restore(Class);
    }

    void FLuaOverrides::Suspend(UClass* Class)
    {
        if (const auto Exists = Overrides.Find(Class))
        {
            if ( Exists != nullptr && Exists->IsValid() )
            {
                Exists->Get()->SetActive(false);
            }
        }
    }

    void FLuaOverrides::Resume(UClass* Class)
    {
        if (const auto Exists = Overrides.Find(Class))
        {
            if ( Exists != nullptr && Exists->IsValid() )
            {
                Exists->Get()->SetActive(true);
            }
        }
    }

    UClass* FLuaOverrides::GetOrAddOverridesClass(UClass* Class)
    {
        const auto Exists = Overrides.Find(Class);
        if ( Exists != nullptr && Exists->IsValid() )
            return Exists->Get();

        const auto OverridesClass = ULuaOverridesClass::Create(Class);
        Overrides.Add(Class, OverridesClass);
        return OverridesClass;
    }
}
