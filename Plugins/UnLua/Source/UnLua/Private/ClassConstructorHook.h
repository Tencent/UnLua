#pragma once

class FClassConstructorHook
{
public:
    static FClassConstructorHook& Get();

    void NotifyUObjectDeleted(const UObjectBase* Object);

    void Hook(UClass* Class);
    
private:
    static void HookedConstructor(const FObjectInitializer& Initializer);

    TMap<UClass*, UClass::ClassConstructorType> OriginalConstructors;
};
