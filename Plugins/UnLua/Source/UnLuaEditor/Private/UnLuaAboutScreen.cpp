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

#include "UnLuaAboutScreen.h"
#include "Misc/EngineVersionComparison.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"
#include "Styling/CoreStyle.h"
#include "EditorStyleSet.h"
#include "UnLuaEditorFunctionLibrary.h"
#include "UnLuaEditorStyle.h"

#define LOCTEXT_NAMESPACE "UnLuaAboutScreen"

void SUnLuaAboutScreen::Construct(const FArguments& InArgs)
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4428)	// universal-character-name encountered in source
#endif
    AboutLines.Add(MakeShareable(new FLineDefinition(LOCTEXT("Copyright1", "https://github.com/Tencent/UnLua"), 8, FLinearColor(1.f, 1.f, 1.f), FMargin(0.f))));
    AboutLines.Add(MakeShareable(new FLineDefinition(LOCTEXT("Copyright2", "Tencent is pleased to support the open source community by making UnLua available."), 8, FLinearColor(1.f, 1.f, 1.f), FMargin(0.0f, 8.0f))));
    AboutLines.Add(MakeShareable(new FLineDefinition(LOCTEXT("Copyright3", "Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved."), 8, FLinearColor(1.f, 1.f, 1.f), FMargin(0.0f, 2.0f))));
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    FText Version = FText::Format(LOCTEXT("VersionLabel", "Version: {0}"), FText::FromString(UUnLuaEditorFunctionLibrary::GetCurrentVersion()));

    ChildSlot
    [
        SNew(SOverlay)
        + SOverlay::Slot()
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                  .AutoWidth()
                  .VAlign(VAlign_Top)
                  .Padding(FMargin(10.f, 10.f, 0.f, 0.f))
                [
                    SAssignNew(LogoButton, SButton)
#if UE_VERSION_OLDER_THAN(5, 1, 0)
                    .ButtonStyle(FEditorStyle::Get(), "NoBorder")
#else
                    .ButtonStyle(FAppStyle::Get(), "NoBorder")
#endif
                    .OnClicked(this, &SUnLuaAboutScreen::OnLogoButtonClicked)
                    [
                        SNew(SImage).Image(FUnLuaEditorStyle::GetInstance()->GetBrush("UnLuaEditor.UnLuaLogo"))
                    ]
                ]
                + SHorizontalBox::Slot()
                  .AutoWidth()
                  .VAlign(VAlign_Top)
                  .Padding(FMargin(10.f, 10.f, 0.f, 0.f))
                + SHorizontalBox::Slot()
                  .FillWidth(1.f)
                  .HAlign(HAlign_Right)
                  .Padding(FMargin(0.f, 52.f, 7.f, 0.f))
                [
                    SNew(SEditableText)
						.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
						.IsReadOnly(true)
						.Text(Version)
                ]
            ]
            + SVerticalBox::Slot()
              .Padding(FMargin(5.f, 5.f, 5.f, 5.f))
              .VAlign(VAlign_Top)
            [
                SNew(SListView<TSharedRef<FLineDefinition>>)
					.ListItemsSource(&AboutLines)
					.OnGenerateRow(this, &SUnLuaAboutScreen::MakeAboutTextItemWidget)
					.SelectionMode(ESelectionMode::None)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                  .HAlign(HAlign_Left)
                  .Padding(FMargin(5.f, 0.f, 5.f, 5.f))
                + SHorizontalBox::Slot()
                  .AutoWidth()
                  .HAlign(HAlign_Right)
                  .VAlign(VAlign_Bottom)
                  .Padding(FMargin(5.f, 0.f, 5.f, 5.f))
                [
                    SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Text(LOCTEXT("Close", "Close"))
						.ButtonColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
						.OnClicked(this, &SUnLuaAboutScreen::OnClose)
                ]
            ]
        ]
    ];
}

TSharedRef<ITableRow> SUnLuaAboutScreen::MakeAboutTextItemWidget(TSharedRef<FLineDefinition> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    if (Item->Text.IsEmpty())
    {
        return
            SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
            .Padding(6.0f)
            [
                SNew(SSpacer)
            ];
    }
    else
    {
        return
            SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
            .Padding(Item->Margin)
            [
                SNew(STextBlock)
				.ColorAndOpacity(Item->TextColor)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", Item->FontSize))
				.Text(Item->Text)
            ];
    }
}

FReply SUnLuaAboutScreen::OnLogoButtonClicked()
{
    const TCHAR* URL = TEXT("cmd");
    const TCHAR* Params = TEXT("/k start https://github.com/Tencent/UnLua");
    FPlatformProcess::ExecProcess(URL, Params, nullptr, nullptr, nullptr);
    return FReply::Handled();
}

FReply SUnLuaAboutScreen::OnClose()
{
    TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
    FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
