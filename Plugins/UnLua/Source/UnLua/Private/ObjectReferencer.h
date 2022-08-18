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

#include "Containers/Set.h"
#include "UObject/GCObject.h"

namespace UnLua
{
    class FObjectReferencer : public FGCObject
    {
    public:
        void Add(UObject* Object)
        {
            if (Object == nullptr)
                return;
            ReferencedObjects.Add(Object);
        }

        // ReSharper disable once CppParameterMayBeConstPtrOrRef
        void Remove(UObject* Object)
        {
            if (Object == nullptr)
                return;
            ReferencedObjects.Remove(Object);
        }

        void Clear()
        {
            return ReferencedObjects.Empty();
        }

        void SetName(const FString& InName)
        {
            Name = InName;
        }

        virtual void AddReferencedObjects(FReferenceCollector& Collector) override
        {
            Collector.AddReferencedObjects(ReferencedObjects);
        }

        virtual FString GetReferencerName() const override
        {
            return Name;
        }

    private:
        TSet<UObject*> ReferencedObjects;
        FString Name = TEXT("FObjectReferencer");
    };
}
