#include "CkWebUmg_Builder.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkWebUmg/FlexPanel/CkWebUmg_FlexPanel_Slate.h"

#include "Brushes/SlateRoundedBoxBrush.h"
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
        ck::webumg::FCkWebUmg_BuildResult& InOutResult)
        -> TSharedRef<SWidget>
    {
        auto Content = TSharedPtr<SWidget>{};

        if (InNode->Children.Num() > 0)
        {
            const auto Panel = SNew(SCk_WebUmgFlexPanel).IrNode(InNode);
            for (const auto& Child : InNode->Children)
            { Panel->AddIrChild(Child, DoBuildNode(Child, InOutResult)); }
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
        const auto& Paint = InNode->Paint;
        const auto HasRadius = Paint.BorderRadius != FVector4f::Zero();
        const auto HasBorder = Paint.BorderWidth != FVector4f::Zero() && Paint.BorderColor.IsSet();

        if (HasRadius || HasBorder)
        {
            // FSlateBrushOutlineSettings corner order (TL, TR, BR, BL) matches the IR's CSS order.
            const auto Fill = Paint.BackgroundColor.IsSet()
                ? FLinearColor{*Paint.BackgroundColor}
                : FLinearColor::Transparent;
            const auto OutlineColor = Paint.BorderColor.IsSet()
                ? FLinearColor{*Paint.BorderColor}
                : FLinearColor::Transparent;
            const auto OutlineWidth = Paint.BorderWidth.X; // uniform; per-side divergence is diagnosed upstream
            const TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateRoundedBoxBrush>(
                Fill,
                FVector4(Paint.BorderRadius.X, Paint.BorderRadius.Y, Paint.BorderRadius.Z, Paint.BorderRadius.W),
                OutlineColor,
                OutlineWidth);
            InOutResult.OwnedBrushes.Add(Brush);

            Result = SNew(SBorder)
                .Padding(0.0f)
                .BorderImage(Brush.Get())
                [
                    Content.ToSharedRef()
                ];
        }
        else if (Paint.BackgroundColor.IsSet())
        {
            Result = SNew(SBorder)
                .Padding(0.0f)
                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FSlateColor{FLinearColor{*Paint.BackgroundColor}})
                [
                    Content.ToSharedRef()
                ];
        }

        if (InNode->Paint.Visibility == TEXT("hidden"))
        { Result->SetVisibility(EVisibility::Hidden); }

        // overflow hidden/auto/scroll all clip paint; scrolling behavior itself is Gate 5 scope.
        const auto Clips = [](const FString& InOverflow)
        { return InOverflow == TEXT("hidden") || InOverflow == TEXT("auto") || InOverflow == TEXT("scroll"); };
        if (Clips(InNode->Layout.OverflowX) || Clips(InNode->Layout.OverflowY))
        { Result->SetClipping(EWidgetClipping::ClipToBounds); }

        Result->SetRenderOpacity(InNode->Paint.Opacity);

        InOutResult.WidgetsByIrId.Add(InNode->Id, Result);
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
        Result.RootWidget = ck_webumg_builder::DoBuildNode(InRoot, Result);
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
