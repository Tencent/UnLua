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

#include "Engine/CollisionProfile.h"
#include "CollisionHelper.h"

TArray<FName> FCollisionHelper::ChannelNames;
UEnum* FCollisionHelper::CollisionChannelEnum;
UEnum* FCollisionHelper::ObjectTypeQueryEnum;
UEnum* FCollisionHelper::TraceTypeQueryEnum;

void FCollisionHelper::Initialize()
{
    CollisionChannelEnum = StaticEnum<ECollisionChannel>();
    ObjectTypeQueryEnum = StaticEnum<EObjectTypeQuery>();
    TraceTypeQueryEnum = StaticEnum<ETraceTypeQuery>();
    check(CollisionChannelEnum && ObjectTypeQueryEnum && TraceTypeQueryEnum);

    if (ChannelNames.Num() > 0)
    {
        return;
    }

    UCollisionProfile *CollisionProfile = UCollisionProfile::Get();
    int32 ContainerIndex = 0;
    while (true)
    {
        FName ChannelName = CollisionProfile->ReturnChannelNameFromContainerIndex(ContainerIndex++);
        if (ChannelName == NAME_None)
        {
            break;
        }
        ChannelNames.Add(ChannelName);
    }
}

void FCollisionHelper::Cleanup()
{
    ChannelNames.Empty();
    CollisionChannelEnum = NULL;
    ObjectTypeQueryEnum = NULL;
    TraceTypeQueryEnum = NULL;
}

int32 FCollisionHelper::ConvertToCollisionChannel(FName Name)           // ECollisionChannel
{
    int32 Index = ChannelNames.Find(Name);
    if (Index == INDEX_NONE)
    {
        Index = CollisionChannelEnum->GetValueByName(Name);
    }
    return (ECollisionChannel)Index;
}

int32 FCollisionHelper::ConvertToObjectType(FName Name)                 // EObjectTypeQuery
{
    int32 Index = ChannelNames.Find(Name);
    if (Index == INDEX_NONE)
    {
        Index = ObjectTypeQueryEnum->GetValueByName(Name);
    }
    return UEngineTypes::ConvertToObjectType((ECollisionChannel)Index);
}

int32 FCollisionHelper::ConvertToTraceType(FName Name)                  // ETraceTypeQuery
{
    int32 Index = ChannelNames.Find(Name);
    if (Index == INDEX_NONE)
    {
        Index = TraceTypeQueryEnum->GetValueByName(Name);
    }
    return UEngineTypes::ConvertToTraceType((ECollisionChannel)Index);
}
