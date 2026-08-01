#include "CkWebUmg_Builder.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkWebUmg_Log.h"
#include "CkWebUmg/FlexPanel/CkWebUmg_FlexPanel_Slate.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
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

    // Gradients bake to a per-node transient texture: every pixel computed with the browser's own
    // math — stop interpolation in sRGB space, CSS gradient-line/ellipse geometry — then round-
    // tripped through an sRGB texture exactly like image assets. Exact for linear (any angle) and
    // radial (typed center+radius); conic/unparseable stays unpainted and diagnosed.
    auto
    SampleGradientSrgb(
        const TArray<FCkWebUmg_IrGradientStop>& InStops,
        const TArray<float>& InResolvedPos,
        float InT)
        -> FColor
    {
        if (InT <= InResolvedPos[0])
        { return InStops[0].Color; }
        if (InT >= InResolvedPos.Last())
        { return InStops.Last().Color; }

        for (auto Index = 0; Index < InStops.Num() - 1; ++Index)
        {
            if (InT > InResolvedPos[Index + 1])
            { continue; }
            const auto Span = InResolvedPos[Index + 1] - InResolvedPos[Index];
            const auto Alpha = Span > KINDA_SMALL_NUMBER ? (InT - InResolvedPos[Index]) / Span : 1.0f;
            const auto& From = InStops[Index].Color;
            const auto& To = InStops[Index + 1].Color;
            return FColor{
                static_cast<uint8>(FMath::RoundToInt(FMath::Lerp(static_cast<float>(From.R), static_cast<float>(To.R), Alpha))),
                static_cast<uint8>(FMath::RoundToInt(FMath::Lerp(static_cast<float>(From.G), static_cast<float>(To.G), Alpha))),
                static_cast<uint8>(FMath::RoundToInt(FMath::Lerp(static_cast<float>(From.B), static_cast<float>(To.B), Alpha))),
                static_cast<uint8>(FMath::RoundToInt(FMath::Lerp(static_cast<float>(From.A), static_cast<float>(To.A), Alpha)))};
        }
        return InStops.Last().Color;
    }

    auto
    BakeGradientBrush(
        FBuildContext& InCtx,
        const FCkWebUmg_IrGradient& InGradient,
        const FString& InNodeId,
        const FVector2f InSize)
        -> const FSlateBrush*
    {
        const auto Width = FMath::RoundToInt32(InSize.X);
        const auto Height = FMath::RoundToInt32(InSize.Y);
        if (Width <= 0 || Height <= 0 || InGradient.Stops.Num() < 2)
        { return nullptr; }

        const auto IsLinear = InGradient.GradientType == TEXT("linear") && InGradient.AngleDeg.IsSet();
        const auto IsRadial = InGradient.GradientType == TEXT("radial")
            && InGradient.RadialCenter.IsSet() && InGradient.RadialRadius.IsSet()
            && InGradient.RadialRadius->X > KINDA_SMALL_NUMBER && InGradient.RadialRadius->Y > KINDA_SMALL_NUMBER;
        if (NOT IsLinear && NOT IsRadial)
        {
            ck::webumg::Warning(
                TEXT("Node [{}] gradient type [{}] is not paintable (missing typed geometry) — left unpainted"),
                InNodeId, InGradient.GradientType);
            return nullptr;
        }

        // CSS stop-position resolution: unset first/last pin to 0/100, interior unset distribute
        // evenly between their positioned neighbors, positions clamp non-decreasing.
        auto Positions = TArray<float>{};
        Positions.SetNum(InGradient.Stops.Num());
        for (auto Index = 0; Index < InGradient.Stops.Num(); ++Index)
        {
            Positions[Index] = InGradient.Stops[Index].PosPct.IsSet()
                ? *InGradient.Stops[Index].PosPct / 100.0f
                : (Index == 0 ? 0.0f : Index == InGradient.Stops.Num() - 1 ? 1.0f : -1.0f);
        }
        for (auto Index = 1; Index < Positions.Num(); ++Index)
        {
            if (Positions[Index] >= 0.0f)
            {
                Positions[Index] = FMath::Max(Positions[Index], Positions[Index - 1]);
                continue;
            }
            auto NextSet = Index + 1;
            while (Positions[NextSet] < 0.0f) { ++NextSet; }
            const auto Step = (Positions[NextSet] - Positions[Index - 1]) / static_cast<float>(NextSet - Index + 1);
            for (auto Fill = Index; Fill < NextSet; ++Fill)
            { Positions[Fill] = Positions[Index - 1] + Step * static_cast<float>(Fill - Index + 1); }
        }

        auto Pixels = TArray<FColor>{};
        Pixels.SetNumUninitialized(Width * Height);

        auto DirX = 0.0f, DirY = 0.0f, LineLength = 1.0f;
        if (IsLinear)
        {
            const auto Radians = FMath::DegreesToRadians(*InGradient.AngleDeg);
            DirX = FMath::Sin(Radians);
            DirY = -FMath::Cos(Radians); // CSS 0deg points up; pixel Y grows down
            LineLength = FMath::Abs(InSize.X * DirX) + FMath::Abs(InSize.Y * DirY);
        }

        for (auto Y = 0; Y < Height; ++Y)
        {
            for (auto X = 0; X < Width; ++X)
            {
                const auto Px = static_cast<float>(X) + 0.5f;
                const auto Py = static_cast<float>(Y) + 0.5f;
                auto T = 0.0f;
                if (IsLinear)
                {
                    T = ((Px - InSize.X * 0.5f) * DirX + (Py - InSize.Y * 0.5f) * DirY) / LineLength + 0.5f;
                }
                else
                {
                    const auto Dx = (Px - InGradient.RadialCenter->X) / InGradient.RadialRadius->X;
                    const auto Dy = (Py - InGradient.RadialCenter->Y) / InGradient.RadialRadius->Y;
                    T = FMath::Sqrt(Dx * Dx + Dy * Dy);
                }
                // FColor's in-memory layout on this platform is already B,G,R,A — matches PF_B8G8R8A8.
                Pixels[Y * Width + X] = SampleGradientSrgb(InGradient.Stops, Positions, T);
            }
        }

        auto* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
        if (Texture == nullptr)
        { return nullptr; }
        Texture->SRGB = true;
        auto& Mip = Texture->GetPlatformData()->Mips[0];
        auto* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
        FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
        Mip.BulkData.Unlock();
        Texture->UpdateResource();

        InCtx.Result.OwnedTextures.Add(TStrongObjectPtr<UTexture2D>{Texture});
        const TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
        Brush->SetResourceObject(Texture);
        Brush->ImageSize = FVector2D(InSize);
        Brush->DrawAs = ESlateBrushDrawType::Image;
        InCtx.Result.OwnedBrushes.Add(Brush);
        return Brush.Get();
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
                FVector2f(InNode->Get_LayoutBox().Border.W, InNode->Get_LayoutBox().Border.H));
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
            const auto* GradientBrush = BakeGradientBrush(InCtx, *Paint.Gradient, InNode->Id,
                FVector2f(InNode->Get_LayoutBox().Border.W, InNode->Get_LayoutBox().Border.H));
            if (GradientBrush != nullptr)
            {
                Result = SNew(SBorder)
                    .Padding(0.0f)
                    .BorderImage(GradientBrush)
                    [
                        Content.ToSharedRef()
                    ];
            }
        }
        else if (Paint.BackgroundImageAsset.IsSet())
        {
            const auto* BackgroundBrush = LoadAssetBrush(InCtx, *Paint.BackgroundImageAsset,
                FVector2f(InNode->Get_LayoutBox().Border.W, InNode->Get_LayoutBox().Border.H));
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

        if (Paint.Transform.IsSet())
        {
            if (Paint.Transform->Matrix.Num() == 6)
            {
                // CSS applies p' = origin + M·(p−origin) + t; Slate's render transform about the
                // pivot is the same composition, with the pivot normalized to the widget's
                // UNtransformed layout size (the transform reapplies over layout geometry).
                const auto& M = Paint.Transform->Matrix;
                const auto& LayoutBorder = InNode->Get_LayoutBox().Border;
                Result->SetRenderTransform(FSlateRenderTransform(
                    FMatrix2x2{M[0], M[1], M[2], M[3]}, FVector2D(M[4], M[5])));
                Result->SetRenderTransformPivot(FVector2D(
                    LayoutBorder.W > 0.0f ? Paint.Transform->Origin.X / LayoutBorder.W : 0.5f,
                    LayoutBorder.H > 0.0f ? Paint.Transform->Origin.Y / LayoutBorder.H : 0.5f));
            }
            else
            {
                ck::webumg::Warning(
                    TEXT("Node [{}] has a 3D transform the IR could not type — left unapplied"),
                    InNode->Id);
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
