#include "CkWebUmg_Builder.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkWebUmg_Log.h"
#include "CkWebUmg/FlexPanel/CkWebUmg_FlexPanel_Slate.h"

#include "Algo/Reverse.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Colors/SComplexGradient.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_webumg_builder
{
    struct FBuildContext
    {
        const FCkWebUmg_IrDocument& Document;
        FString ContentBaseDir;
        ck::webumg::FCkWebUmg_BuildResult& Result;
    };

    auto
    LoadAssetBrush(
        FBuildContext& InCtx,
        const FString& InAssetId,
        const FVector2f InDrawSize)
        -> const FSlateBrush*
    {
        if (InCtx.ContentBaseDir.IsEmpty())
        { return nullptr; }

        const auto* Src = InCtx.Document.AssetSourcesById.Find(InAssetId);
        if (Src == nullptr)
        {
            ck::webumg::Warning(TEXT("Asset id [{}] not in document asset table"), InAssetId);
            return nullptr;
        }

        const auto Path = FPaths::Combine(InCtx.ContentBaseDir, *Src);
        if (NOT FPaths::FileExists(Path))
        {
            ck::webumg::Warning(TEXT("Asset file missing: [{}]"), Path);
            return nullptr;
        }

        auto* Texture = FImageUtils::ImportFileAsTexture2D(Path);
        if (Texture == nullptr)
        {
            ck::webumg::Warning(TEXT("ImportFileAsTexture2D failed for [{}]"), Path);
            return nullptr;
        }
        // sRGB-authored pixels in the linear pipeline: decode-on-sample + encode-on-write round-
        // trips opaque content exactly. Translucent assets diverge by design — UE composites in
        // linear space, the browser in sRGB (§8 written position; measured: α=0.5 red over
        // #0c0e12 → browser 133, UE 188).
        Texture->SRGB = true;
        Texture->UpdateResource();
        ck::webumg::Display(TEXT("Loaded asset [{}] from [{}]"), InAssetId, Path);

        InCtx.Result.OwnedTextures.Add(TStrongObjectPtr<UTexture2D>{Texture});
        const TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
        Brush->SetResourceObject(Texture);
        Brush->ImageSize = FVector2D(InDrawSize);
        Brush->DrawAs = ESlateBrushDrawType::Image;
        InCtx.Result.OwnedBrushes.Add(Brush);
        return Brush.Get();
    }

    // Axis-aligned linear gradients with even (or unspecified) stop spacing map exactly onto
    // SComplexGradient; everything else stays unpainted and measured — material-backed gradients
    // are a Gate 3 strategy decision, not a silent approximation.
    auto
    TryMakeGradientWidget(
        const FCkWebUmg_IrGradient& InGradient)
        -> TSharedPtr<SWidget>
    {
        if (InGradient.GradientType != TEXT("linear") || NOT InGradient.AngleDeg.IsSet())
        { return {}; }

        const auto Angle = FMath::Fmod(FMath::Fmod(*InGradient.AngleDeg, 360.0f) + 360.0f, 360.0f);
        const auto IsVertical = FMath::IsNearlyEqual(Angle, 180.0f) || FMath::IsNearlyEqual(Angle, 0.0f);
        const auto IsHorizontal = FMath::IsNearlyEqual(Angle, 90.0f) || FMath::IsNearlyEqual(Angle, 270.0f);
        if (NOT IsVertical && NOT IsHorizontal)
        { return {}; }

        const auto StopCount = InGradient.Stops.Num();
        for (auto Index = 0; Index < StopCount; ++Index)
        {
            const auto& Stop = InGradient.Stops[Index];
            if (Stop.PosPct.IsSet() &&
                NOT FMath::IsNearlyEqual(*Stop.PosPct, Index * (100.0f / (StopCount - 1)), 0.5f))
            { return {}; } // uneven spacing — SComplexGradient distributes evenly
        }

        auto Colors = TArray<FLinearColor>{};
        for (const auto& Stop : InGradient.Stops)
        { Colors.Add(FLinearColor{Stop.Color}); }

        // 0deg = to top, 270deg = to left — stop order reverses against the draw direction.
        if (FMath::IsNearlyEqual(Angle, 0.0f) || FMath::IsNearlyEqual(Angle, 270.0f))
        { Algo::Reverse(Colors); }

        // SComplexGradient's Orientation names the BAND axis, not the color-variation axis
        // (verified from the rendered dump: Orient_Vertical varies horizontally) — hence swapped.
        return SNew(SComplexGradient)
            .GradientColors(Colors)
            .Orientation(IsVertical ? Orient_Horizontal : Orient_Vertical);
    }

    auto
    DoBuildNode(
        const TSharedPtr<const FCkWebUmg_IrNode>& InNode,
        FBuildContext& InCtx)
        -> TSharedRef<SWidget>
    {
        auto& InOutResult = InCtx.Result;
        auto Content = TSharedPtr<SWidget>{};

        if (InNode->Children.Num() > 0)
        {
            const auto Panel = SNew(SCk_WebUmgFlexPanel).IrNode(InNode);
            for (const auto& Child : InNode->Children)
            { Panel->AddIrChild(Child, DoBuildNode(Child, InCtx)); }
            Content = Panel;
        }
        else if (NOT InNode->Asset.IsEmpty())
        {
            const auto* ImageBrush = LoadAssetBrush(InCtx, InNode->Asset,
                FVector2f(InNode->Box.Border.W, InNode->Box.Border.H));
            Content = ImageBrush != nullptr
                ? TSharedPtr<SWidget>{SNew(SImage).Image(ImageBrush)}
                : TSharedPtr<SWidget>{SNew(SBox)};
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
        else if (Paint.Gradient.IsSet())
        {
            if (const auto GradientWidget = TryMakeGradientWidget(*Paint.Gradient); GradientWidget != nullptr)
            {
                Result = SNew(SOverlay)
                    + SOverlay::Slot()[GradientWidget.ToSharedRef()]
                    + SOverlay::Slot()[Content.ToSharedRef()];
            }
        }
        else if (Paint.BackgroundImageAsset.IsSet())
        {
            const auto* BackgroundBrush = LoadAssetBrush(InCtx, *Paint.BackgroundImageAsset,
                FVector2f(InNode->Box.Border.W, InNode->Box.Border.H));
            if (BackgroundBrush != nullptr)
            {
                Result = SNew(SBorder)
                    .Padding(0.0f)
                    .BorderImage(BackgroundBrush)
                    [
                        Content.ToSharedRef()
                    ];
            }
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
        const FCkWebUmg_IrDocument& InDocument,
        const FString& InContentBaseDir)
        -> FCkWebUmg_BuildResult
    {
        const auto RootIsValid = InDocument.Root != nullptr;
        CK_ENSURE_IF_NOT(RootIsValid, TEXT("BuildWidgetTree called with a document that has no root"))
        {}
        if (NOT RootIsValid)
        { return {}; }

        auto Result = FCkWebUmg_BuildResult{};
        auto Context = ck_webumg_builder::FBuildContext{InDocument, InContentBaseDir, Result};
        Result.RootWidget = ck_webumg_builder::DoBuildNode(InDocument.Root, Context);
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
