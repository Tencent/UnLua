#include "ClassConstructorHook.h"
#include "UnLuaModule.h"

FClassConstructorHook& FClassConstructorHook::Get()
{
    static FClassConstructorHook Instance;
    return Instance;
}

void FClassConstructorHook::NotifyUObjectDeleted(const UObjectBase* Object)
{
    OriginalConstructors.Remove((UClass*)Object);
}

void FClassConstructorHook::Hook(UClass* Class)
{
    const auto Exists = OriginalConstructors.Find(Class);
    if (Exists)
        return;

    OriginalConstructors.Add(Class, Class->ClassConstructor);
    Class->ClassConstructor = &HookedConstructor;
}

void FClassConstructorHook::HookedConstructor(const FObjectInitializer& Initializer)
{
    const auto Class = Initializer.GetClass();
    const auto OriginalConstructor = Get().OriginalConstructors.FindChecked(Class);
    OriginalConstructor(Initializer);
    const auto Env = IUnLuaModule::Get().GetEnv(Class);
    Env->GetManager()->ConstructObject(Initializer);
}
