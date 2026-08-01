#include "CkWebUmg_Builder.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkWebUmg/FlexPanel/CkWebUmg_FlexPanel_Slate.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_webumg_builder
{
    auto
    DoBuildNode(
        const TSharedPtr<const FCkWebUmg_IrNode>& InNode,
        TMap<FString, TSharedPtr<SWidget>>& InOutWidgetsById)
        -> TSharedRef<SWidget>
    {
        auto Content = TSharedPtr<SWidget>{};

        if (InNode->Children.Num() > 0)
        {
            const auto Panel = SNew(SCk_WebUmgFlexPanel).IrNode(InNode);
            for (const auto& Child : InNode->Children)
            { Panel->AddIrChild(Child, DoBuildNode(Child, InOutWidgetsById)); }
            Content = Panel;
        }
        else if (InNode->Text.IsSet())
        {
            const auto& Text = *InNode->Text;
            const auto FontInfo = FCoreStyle::GetDefaultFontStyle(
                Text.Weight >= 600 ? "Bold" : "Regular", FMath::RoundToInt32(Text.SizePx));
            Content = SNew(STextBlock)
                .Text(FText::FromString(Text.Content))
                .Font(FontInfo)
                .ColorAndOpacity(Text.Color.IsSet()
                    ? FSlateColor{FLinearColor{*Text.Color}}
                    : FSlateColor::UseForeground());
        }
        else
        {
            // Never the SNullWidget singleton — every IR node needs its own mutable widget
            // identity for visibility/opacity and for the harness id map.
            Content = SNew(SBox);
        }

        auto Result = Content.ToSharedRef();
        if (InNode->Paint.BackgroundColor.IsSet())
        {
            Result = SNew(SBorder)
                .Padding(0.0f)
                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FSlateColor{FLinearColor{*InNode->Paint.BackgroundColor}})
                [
                    Content.ToSharedRef()
                ];
        }

        if (InNode->Paint.Visibility == TEXT("hidden"))
        { Result->SetVisibility(EVisibility::Hidden); }

        Result->SetRenderOpacity(InNode->Paint.Opacity);

        InOutWidgetsById.Add(InNode->Id, Result);
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::webumg
{
    auto
    BuildWidgetTree(
        const TSharedPtr<const FCkWebUmg_IrNode>& InRoot)
        -> FCkWebUmg_BuildResult
    {
        const auto RootIsValid = InRoot != nullptr;
        CK_ENSURE_IF_NOT(RootIsValid, TEXT("BuildWidgetTree called with a null IR root"))
        {}
        if (NOT RootIsValid)
        { return {}; }

        auto Result = FCkWebUmg_BuildResult{};
        Result.RootWidget = ck_webumg_builder::DoBuildNode(InRoot, Result.WidgetsByIrId);
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
