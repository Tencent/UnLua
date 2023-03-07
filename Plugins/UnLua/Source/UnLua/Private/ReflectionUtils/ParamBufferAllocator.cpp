#include "ParamBufferAllocator.h"

#include "UnLuaPrivate.h"

FParamBufferAllocator_Always::FParamBufferAllocator_Always(const UFunction& Func)
{
    check(Func.ParmsSize);
    ParmsSize = Func.ParmsSize;
}

void* FParamBufferAllocator_Always::Get()
{
    const auto Ret = FMemory::Malloc(ParmsSize, 16);
    FMemory::Memzero(Ret, ParmsSize);
    return Ret;
}

void FParamBufferAllocator_Always::Pop(void* Memory)
{
    FMemory::Free(Memory);
}

FParamBufferAllocator_Persistent::FParamBufferAllocator_Persistent(const UFunction& Func)
    : Counter(0)
{
    ParmsSize = Func.ParmsSize;
}

FParamBufferAllocator_Persistent::~FParamBufferAllocator_Persistent()
{
    for (int i = 0; i < Buffers.Num(); ++i)
    {
        UNLUA_STAT_MEMORY_FREE(Buffers[i], PersistentParamBuffer);
        FMemory::Free(Buffers[i]);
    }
}

void* FParamBufferAllocator_Persistent::Get()
{
    if (Counter < Buffers.Num())
        return Buffers[Counter++];

    Counter++;
    const auto Buffer = FMemory::Malloc(ParmsSize, 16);
    FMemory::Memzero(Buffer, ParmsSize);
    UNLUA_STAT_MEMORY_ALLOC(Buffer, PersistentParamBuffer);
    Buffers.Add(Buffer);
    return Buffer;
}

void FParamBufferAllocator_Persistent::Pop(void* Memory)
{
    check(Counter > 0);
    Counter--;
    check(Buffers[Counter] == Memory);
}

TSharedRef<FParamBufferAllocator> FParamBufferFactory::Get(const UFunction& Func)
{
    if (Func.ParmsSize == 0)
    {
        static FParamBufferAllocator_Empty Empty;
        return MakeShared<FParamBufferAllocator_Empty>(Empty);
    }

#if ENABLE_PERSISTENT_PARAM_BUFFER
    return MakeShareable(new FParamBufferAllocator_Persistent(Func));
#else
    return MakeShareable(new FParamBufferAllocator_Always(Func));
#endif
}
