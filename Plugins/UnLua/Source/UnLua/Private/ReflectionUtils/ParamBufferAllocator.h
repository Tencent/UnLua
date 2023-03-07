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

class FParamBufferAllocator
{
public:
    virtual ~FParamBufferAllocator() = default;

    virtual void* Get() = 0;

    virtual void Pop(void* Memory) = 0;
};

class FParamBufferAllocator_Empty : public FParamBufferAllocator
{
public:
    virtual void* Get() override
    {
        return nullptr;
    }

    virtual void Pop(void* Memory) override
    {
    }
};

class FParamBufferAllocator_Always : public FParamBufferAllocator
{
public:
    explicit FParamBufferAllocator_Always(const UFunction& Func);

    virtual void* Get() override;

    virtual void Pop(void* Memory) override;

private:
    uint16 ParmsSize;
};

class FParamBufferAllocator_Persistent : public FParamBufferAllocator
{
public:
    explicit FParamBufferAllocator_Persistent(const UFunction& Func);

    virtual ~FParamBufferAllocator_Persistent() override;

    virtual void* Get() override;

    virtual void Pop(void* Memory) override;

private:
    uint8 Counter;
    uint16 ParmsSize;
    TArray<void*> Buffers;
};

class FParamBufferFactory
{
public:
    static TSharedRef<FParamBufferAllocator> Get(const UFunction& Func);
};
