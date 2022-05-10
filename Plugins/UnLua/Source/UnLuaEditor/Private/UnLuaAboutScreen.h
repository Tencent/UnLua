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

#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class ITableRow;
class SButton;
class STableViewBase;
struct FSlateBrush;

class SUnLuaAboutScreen : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SUnLuaAboutScreen)
        {
        }

    SLATE_END_ARGS()

    UNLUAEDITOR_API void Construct(const FArguments& InArgs);

private:
    struct FLineDefinition
    {
    public:
        FText Text;
        int32 FontSize;
        FLinearColor TextColor;
        FMargin Margin;

        FLineDefinition(const FText& InText)
            : Text(InText)
              , FontSize(9)
              , TextColor(FLinearColor(0.5f, 0.5f, 0.5f))
              , Margin(FMargin(6.f, 0.f, 0.f, 0.f))
        {
        }

        FLineDefinition(const FText& InText, int32 InFontSize, const FLinearColor& InTextColor, const FMargin& InMargin)
            : Text(InText)
              , FontSize(InFontSize)
              , TextColor(InTextColor)
              , Margin(InMargin)
        {
        }
    };

    TArray<TSharedRef<FLineDefinition>> AboutLines;
    TSharedPtr<SButton> LogoButton;

    TSharedRef<ITableRow> MakeAboutTextItemWidget(TSharedRef<FLineDefinition> Item, const TSharedRef<STableViewBase>& OwnerTable);

    FReply OnLogoButtonClicked();
    FReply OnClose();
};
