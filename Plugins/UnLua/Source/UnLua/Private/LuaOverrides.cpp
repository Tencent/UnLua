#include "Misc/EngineVersionComparison.h"
#include "Engine/BlueprintGeneratedClass.h"
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

    void FLuaOverrides::NotifyUObjectDeleted(const UObjectBase* Object, int32 Index)
    {
        ULuaOverridesClass* OverridesClass;
        if (!Overrides.RemoveAndCopyValue((UClass*)Object, OverridesClass))
            return;
        OverridesClass->RemoveFromRoot();
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

        FObjectDuplicationParameters DuplicationParams(Function, OverridesClass);
        DuplicationParams.InternalFlagMask &= ~EInternalObjectFlags::Native;
        DuplicationParams.DestName = NewName;
        DuplicationParams.DestClass = ULuaFunction::StaticClass();
        LuaFunction = static_cast<ULuaFunction*>(StaticDuplicateObjectEx(DuplicationParams));

        LuaFunction->Next = OverridesClass->Children;
        OverridesClass->Children = LuaFunction;

        LuaFunction->StaticLink(true);
        LuaFunction->Initialize();
        LuaFunction->Override(Function, Class, bAddNew);
    }

    void FLuaOverrides::Restore(UClass* Class)
    {
        const auto Exists = Overrides.Find(Class);
        if (!Exists)
            return;

        const auto OverridesClass = *Exists;
        OverridesClass->Restore();
        OverridesClass->RemoveFromRoot();
        Overrides.Remove(Class);
    }

    void FLuaOverrides::RestoreAll()
    {
        TArray<UClass*> Classes;
        Overrides.GenerateKeyArray(Classes);
        for (const auto& Class : Classes)
            Restore(Class);
    }

    UClass* FLuaOverrides::GetOrAddOverridesClass(UClass* Class)
    {
        const auto Exists = Overrides.Find(Class);
        if (Exists)
            return *Exists;

        auto ClassName = FString::Printf(TEXT("LUA_OVERRIDES_%s"), *Class->GetName());
        auto OverridesClass = NewObject<ULuaOverridesClass>(GetTransientPackage(), *ClassName, RF_Public | RF_Transient);
        OverridesClass->AddToRoot();
        OverridesClass->Owner = Class;
        OverridesClass->Next = nullptr;

#if UE_VERSION_OLDER_THAN(5, 1, 0)
        OverridesClass->ClassAddReferencedObjects = Class->AddReferencedObjects;
#else
        OverridesClass->CppClassStaticFunctions = Class->CppClassStaticFunctions;
#endif
        OverridesClass->ClassFlags |= CLASS_CompiledFromBlueprint;
#if WITH_EDITOR
        OverridesClass->ClassGeneratedBy = Class->ClassGeneratedBy;
#endif

        Overrides.Add(Class, OverridesClass);
        return OverridesClass;
    }
}
