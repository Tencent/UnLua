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

#include "DefaultParamCollection.h"
#include "Misc/EngineVersionComparison.h"
#include "CoreUObject.h"

TMap<FName, FFunctionCollection> GDefaultParamCollection;

#if UE_VERSION_OLDER_THAN(5, 2, 0)
PRAGMA_DISABLE_OPTIMIZATION
#else
UE_DISABLE_OPTIMIZATION
#endif

void CreateDefaultParamCollection()
{
    static bool CollectionCreated = false;
    if (!CollectionCreated)
    {
        CollectionCreated = true;

#include "DefaultParamCollection.inl"
    }
}

#if UE_VERSION_OLDER_THAN(5, 2, 0)
PRAGMA_ENABLE_OPTIMIZATION
#else
UE_ENABLE_OPTIMIZATION
#endif
