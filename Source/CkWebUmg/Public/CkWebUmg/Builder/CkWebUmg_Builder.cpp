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

    // The goldens were rendered by system Chrome with the OS font the page asked for, so the only
    // mapping that can converge is the same OS face: first family in the computed stack, resolved
    // against C:/Windows/Fonts (bold cut for weight >= 600). Anything unresolvable falls back to
    // the engine default — visibly wrong metrics, but never silent (Display log names the miss).
    // CSS letter-spacing (px) maps to FSlateFontInfo::LetterSpacing (1/1000 em).
    auto
    MakeFontInfo(
        const FCkWebUmg_IrText& InText)
        -> FSlateFontInfo
    {
        const auto FirstFamily = [&]() -> FString
        {
            auto Family = FString{};
            InText.Family.Split(TEXT(","), &Family, nullptr);
            if (Family.IsEmpty())
            { Family = InText.Family; }
            return Family.TrimStartAndEnd().Replace(TEXT("\""), TEXT(""));
        }();

        const auto IsBold = InText.Weight >= 600;
        static const TMap<FString, TPair<FString, FString>> OsFontFiles = {
            {TEXT("Arial"), {TEXT("arial.ttf"), TEXT("arialbd.ttf")}},
            {TEXT("Segoe UI"), {TEXT("segoeui.ttf"), TEXT("segoeuib.ttf")}},
            {TEXT("Verdana"), {TEXT("verdana.ttf"), TEXT("verdanab.ttf")}},
        };

        // Slate font sizes are POINTS rendered at 96 dpi (drawn em = size * 96/72); CSS sizes are
        // px. Measured before this factor: uniform +34.9% advance/height inflation (1.35 ≈ 4/3).
        const auto SizePt = InText.SizePx * 0.75f;

        auto FontInfo = FSlateFontInfo{};
        if (const auto* Files = OsFontFiles.Find(FirstFamily))
        {
            const auto Path = FPaths::Combine(TEXT("C:/Windows/Fonts"),
                IsBold ? Files->Value : Files->Key);
            if (FPaths::FileExists(Path))
            { FontInfo = FSlateFontInfo{Path, SizePt}; }
        }
        if (NOT FontInfo.HasValidFont())
        {
            ck::webumg::Display(
                TEXT("No OS font mapping for family [{}] — engine default metrics will diverge"),
                FirstFamily);
            FontInfo = FCoreStyle::GetDefaultFontStyle(
                IsBold ? "Bold" : "Regular", FMath::RoundToInt32(SizePt));
        }
        if (InText.LetterSpacingPx != 0.0f && InText.SizePx > 0.0f)
        { FontInfo.LetterSpacing = FMath::RoundToInt32(InText.LetterSpacingPx / InText.SizePx * 1000.0f); }
        return FontInfo;
    }

    auto
    MakeTextureBrush(
        FBuildContext& InCtx,
        const TArray<FColor>& InPixels,
        int32 InWidth,
        int32 InHeight)
        -> const FSlateBrush*
    {
        auto* Texture = UTexture2D::CreateTransient(InWidth, InHeight, PF_B8G8R8A8);
        if (Texture == nullptr)
        { return nullptr; }
        Texture->SRGB = true;
        auto& Mip = Texture->GetPlatformData()->Mips[0];
        auto* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
        FMemory::Memcpy(Data, InPixels.GetData(), InPixels.Num() * sizeof(FColor));
        Mip.BulkData.Unlock();
        Texture->UpdateResource();

        InCtx.Result.OwnedTextures.Add(TStrongObjectPtr<UTexture2D>{Texture});
        const TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
        Brush->SetResourceObject(Texture);
        Brush->ImageSize = FVector2D(InWidth, InHeight);
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

        return MakeTextureBrush(InCtx, Pixels, Width, Height);
    }

    // Signed distance to a rounded rect (per-corner radii, CSS order tl/tr/br/bl); coverage is
    // clamp(0.5 - d, 0, 1) — the same analytic antialias the gradient path relies on textures for.
    auto
    RoundedBoxCoverage(
        const FVector2f InPoint,
        const FVector2f InCenter,
        const FVector2f InHalfSize,
        const FVector4f InRadii)
        -> float
    {
        const auto P = InPoint - InCenter;
        const auto Radius = P.X > 0.0f
            ? (P.Y > 0.0f ? InRadii.Z : InRadii.Y)
            : (P.Y > 0.0f ? InRadii.W : InRadii.X);
        const auto Q = FVector2f(FMath::Abs(P.X), FMath::Abs(P.Y)) - InHalfSize + FVector2f(Radius, Radius);
        const auto Distance = FMath::Min(FMath::Max(Q.X, Q.Y), 0.0f)
            + FVector2f(FMath::Max(Q.X, 0.0f), FMath::Max(Q.Y, 0.0f)).Size() - Radius;
        return FMath::Clamp(0.5f - Distance, 0.0f, 1.0f);
    }

    // Separable Gaussian with the browser's shadow model: sigma = blur/2 (Skia), kernel cut at 3σ.
    auto
    GaussianBlurMask(
        TArray<float>& InOutMask,
        int32 InWidth,
        int32 InHeight,
        float InSigma)
        -> void
    {
        if (InSigma <= KINDA_SMALL_NUMBER)
        { return; }

        const auto KernelRadius = FMath::CeilToInt32(3.0f * InSigma);
        auto Kernel = TArray<float>{};
        Kernel.SetNum(2 * KernelRadius + 1);
        auto Sum = 0.0f;
        for (auto Index = -KernelRadius; Index <= KernelRadius; ++Index)
        {
            const auto Value = FMath::Exp(-0.5f * FMath::Square(static_cast<float>(Index) / InSigma));
            Kernel[Index + KernelRadius] = Value;
            Sum += Value;
        }
        for (auto& Value : Kernel)
        { Value /= Sum; }

        auto Scratch = TArray<float>{};
        Scratch.SetNumZeroed(InOutMask.Num());
        for (auto Y = 0; Y < InHeight; ++Y)
        {
            for (auto X = 0; X < InWidth; ++X)
            {
                auto Acc = 0.0f;
                for (auto K = -KernelRadius; K <= KernelRadius; ++K)
                {
                    const auto Sample = FMath::Clamp(X + K, 0, InWidth - 1);
                    Acc += InOutMask[Y * InWidth + Sample] * Kernel[K + KernelRadius];
                }
                Scratch[Y * InWidth + X] = Acc;
            }
        }
        for (auto X = 0; X < InWidth; ++X)
        {
            for (auto Y = 0; Y < InHeight; ++Y)
            {
                auto Acc = 0.0f;
                for (auto K = -KernelRadius; K <= KernelRadius; ++K)
                {
                    const auto Sample = FMath::Clamp(Y + K, 0, InHeight - 1);
                    Acc += Scratch[Sample * InWidth + X] * Kernel[K + KernelRadius];
                }
                InOutMask[Y * InWidth + X] = Acc;
            }
        }
    }

    // Composite shadow layers back-to-front (CSS: first layer topmost) in straight-alpha sRGB
    // space — the browser blends shadows in sRGB (sec. 8), so the baked texel IS its pixel value.
    struct FShadowAccum
    {
        TArray<FVector4f> Texels; // R,G,B (sRGB 0-255) + A (0-1), straight alpha

        auto CompositeUnder(const TArray<float>& InMask, const FColor InColor) -> void
        {
            const auto LayerAlphaScale = static_cast<float>(InColor.A) / 255.0f;
            for (auto Index = 0; Index < Texels.Num(); ++Index)
            {
                const auto SrcA = InMask[Index] * LayerAlphaScale;
                if (SrcA <= 0.0f)
                { continue; }
                auto& Dst = Texels[Index];
                const auto OutA = Dst.W + SrcA * (1.0f - Dst.W);
                if (OutA <= KINDA_SMALL_NUMBER)
                { continue; }
                // Layers iterate topmost-first, so each new layer goes UNDER the accumulated result.
                Dst.X = (Dst.X * Dst.W + static_cast<float>(InColor.R) * SrcA * (1.0f - Dst.W)) / OutA;
                Dst.Y = (Dst.Y * Dst.W + static_cast<float>(InColor.G) * SrcA * (1.0f - Dst.W)) / OutA;
                Dst.Z = (Dst.Z * Dst.W + static_cast<float>(InColor.B) * SrcA * (1.0f - Dst.W)) / OutA;
                Dst.W = OutA;
            }
        }

        auto ToPixels() const -> TArray<FColor>
        {
            auto Pixels = TArray<FColor>{};
            Pixels.SetNumUninitialized(Texels.Num());
            for (auto Index = 0; Index < Texels.Num(); ++Index)
            {
                const auto& Texel = Texels[Index];
                Pixels[Index] = FColor{
                    static_cast<uint8>(FMath::RoundToInt(FMath::Clamp(Texel.X, 0.0f, 255.0f))),
                    static_cast<uint8>(FMath::RoundToInt(FMath::Clamp(Texel.Y, 0.0f, 255.0f))),
                    static_cast<uint8>(FMath::RoundToInt(FMath::Clamp(Texel.Z, 0.0f, 255.0f))),
                    static_cast<uint8>(FMath::RoundToInt(FMath::Clamp(Texel.W * 255.0f, 0.0f, 255.0f)))};
            }
            return Pixels;
        }
    };

    struct FBakedShadows
    {
        const FSlateBrush* OutsetBrush = nullptr;
        FMargin OutsetMargins; // px the outset canvas extends beyond the node on each side
        const FSlateBrush* InsetBrush = nullptr;
    };

    auto
    BakeShadowBrushes(
        FBuildContext& InCtx,
        const FCkWebUmg_IrPaint& InPaint,
        const FVector2f InSize)
        -> FBakedShadows
    {
        auto Result = FBakedShadows{};
        const auto Width = FMath::RoundToInt32(InSize.X);
        const auto Height = FMath::RoundToInt32(InSize.Y);
        if (Width <= 0 || Height <= 0)
        { return Result; }

        auto OutsetLayers = TArray<FCkWebUmg_IrShadowLayer>{};
        auto InsetLayers = TArray<FCkWebUmg_IrShadowLayer>{};
        for (const auto& Layer : InPaint.ShadowLayers)
        { (Layer.Inset ? InsetLayers : OutsetLayers).Add(Layer); }

        if (OutsetLayers.Num() > 0)
        {
            auto MarginL = 0.0f, MarginT = 0.0f, MarginR = 0.0f, MarginB = 0.0f;
            for (const auto& Layer : OutsetLayers)
            {
                const auto Reach = Layer.Spread + 1.5f * Layer.Blur;
                MarginL = FMath::Max(MarginL, Reach - Layer.Offset.X);
                MarginR = FMath::Max(MarginR, Reach + Layer.Offset.X);
                MarginT = FMath::Max(MarginT, Reach - Layer.Offset.Y);
                MarginB = FMath::Max(MarginB, Reach + Layer.Offset.Y);
            }
            MarginL = FMath::CeilToFloat(FMath::Max(MarginL, 0.0f));
            MarginT = FMath::CeilToFloat(FMath::Max(MarginT, 0.0f));
            MarginR = FMath::CeilToFloat(FMath::Max(MarginR, 0.0f));
            MarginB = FMath::CeilToFloat(FMath::Max(MarginB, 0.0f));

            const auto CanvasW = Width + static_cast<int32>(MarginL + MarginR);
            const auto CanvasH = Height + static_cast<int32>(MarginT + MarginB);
            auto Accum = FShadowAccum{};
            Accum.Texels.SetNumZeroed(CanvasW * CanvasH);

            for (const auto& Layer : OutsetLayers)
            {
                const auto HalfSize = FVector2f(
                    InSize.X * 0.5f + Layer.Spread, InSize.Y * 0.5f + Layer.Spread);
                const auto Center = FVector2f(
                    MarginL + InSize.X * 0.5f + Layer.Offset.X,
                    MarginT + InSize.Y * 0.5f + Layer.Offset.Y);
                const auto Radii = FVector4f(
                    InPaint.BorderRadius.X > 0.0f ? FMath::Max(InPaint.BorderRadius.X + Layer.Spread, 0.0f) : 0.0f,
                    InPaint.BorderRadius.Y > 0.0f ? FMath::Max(InPaint.BorderRadius.Y + Layer.Spread, 0.0f) : 0.0f,
                    InPaint.BorderRadius.Z > 0.0f ? FMath::Max(InPaint.BorderRadius.Z + Layer.Spread, 0.0f) : 0.0f,
                    InPaint.BorderRadius.W > 0.0f ? FMath::Max(InPaint.BorderRadius.W + Layer.Spread, 0.0f) : 0.0f);

                auto Mask = TArray<float>{};
                Mask.SetNumUninitialized(CanvasW * CanvasH);
                for (auto Y = 0; Y < CanvasH; ++Y)
                {
                    for (auto X = 0; X < CanvasW; ++X)
                    {
                        Mask[Y * CanvasW + X] = RoundedBoxCoverage(
                            FVector2f(static_cast<float>(X) + 0.5f, static_cast<float>(Y) + 0.5f),
                            Center, HalfSize, Radii);
                    }
                }
                GaussianBlurMask(Mask, CanvasW, CanvasH, Layer.Blur * 0.5f);
                Accum.CompositeUnder(Mask, Layer.Color);
            }

            Result.OutsetBrush = MakeTextureBrush(InCtx, Accum.ToPixels(), CanvasW, CanvasH);
            Result.OutsetMargins = FMargin(MarginL, MarginT, MarginR, MarginB);
        }

        if (InsetLayers.Num() > 0)
        {
            auto Accum = FShadowAccum{};
            Accum.Texels.SetNumZeroed(Width * Height);

            for (const auto& Layer : InsetLayers)
            {
                // Inset shadow: blur the COMPLEMENT of the spread-shrunk rect, then clip to the
                // border box's own rounded silhouette.
                const auto HalfSize = FVector2f(
                    FMath::Max(InSize.X * 0.5f - Layer.Spread, 0.0f),
                    FMath::Max(InSize.Y * 0.5f - Layer.Spread, 0.0f));
                const auto Center = FVector2f(
                    InSize.X * 0.5f + Layer.Offset.X, InSize.Y * 0.5f + Layer.Offset.Y);
                const auto Radii = FVector4f(
                    FMath::Max(InPaint.BorderRadius.X - Layer.Spread, 0.0f),
                    FMath::Max(InPaint.BorderRadius.Y - Layer.Spread, 0.0f),
                    FMath::Max(InPaint.BorderRadius.Z - Layer.Spread, 0.0f),
                    FMath::Max(InPaint.BorderRadius.W - Layer.Spread, 0.0f));

                auto Mask = TArray<float>{};
                Mask.SetNumUninitialized(Width * Height);
                for (auto Y = 0; Y < Height; ++Y)
                {
                    for (auto X = 0; X < Width; ++X)
                    {
                        Mask[Y * Width + X] = 1.0f - RoundedBoxCoverage(
                            FVector2f(static_cast<float>(X) + 0.5f, static_cast<float>(Y) + 0.5f),
                            Center, HalfSize, Radii);
                    }
                }
                GaussianBlurMask(Mask, Width, Height, Layer.Blur * 0.5f);
                const auto BoxHalfSize = FVector2f(InSize.X * 0.5f, InSize.Y * 0.5f);
                const auto BoxCenter = BoxHalfSize;
                for (auto Y = 0; Y < Height; ++Y)
                {
                    for (auto X = 0; X < Width; ++X)
                    {
                        Mask[Y * Width + X] *= RoundedBoxCoverage(
                            FVector2f(static_cast<float>(X) + 0.5f, static_cast<float>(Y) + 0.5f),
                            BoxCenter, BoxHalfSize, InPaint.BorderRadius);
                    }
                }
                Accum.CompositeUnder(Mask, Layer.Color);
            }

            Result.InsetBrush = MakeTextureBrush(InCtx, Accum.ToPixels(), Width, Height);
        }

        return Result;
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
            Content = SNew(STextBlock)
                .Text(FText::FromString(ck::webumg::ApplyTextTransform(Text.Content, Text.TransformCase)))
                .Font(MakeFontInfo(Text))
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

        auto BakedShadows = FBakedShadows{};
        if (Paint.ShadowLayers.Num() > 0)
        {
            BakedShadows = BakeShadowBrushes(InCtx, Paint,
                FVector2f(InNode->Get_LayoutBox().Border.W, InNode->Get_LayoutBox().Border.H));
            if (BakedShadows.InsetBrush != nullptr)
            {
                // CSS layering: background, then inset shadow, then content — the background
                // wrapper below draws its brush before children, so nesting here lands it right.
                Content = SNew(SOverlay)
                    + SOverlay::Slot()[SNew(SImage).Image(BakedShadows.InsetBrush)]
                    + SOverlay::Slot()[Content.ToSharedRef()];
                Result = Content.ToSharedRef();
            }
        }
        if (Paint.HasUntypedShadow)
        {
            ck::webumg::Warning(
                TEXT("Node [{}] has a box-shadow the IR could not type — left unpainted"),
                InNode->Id);
        }

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

        if (BakedShadows.OutsetBrush != nullptr)
        {
            // The outset halo paints outside the node rect: a negative-padding overlay slot hands
            // the shadow image a geometry larger than the node (Slate doesn't clip unless asked).
            // Wrapping the FINAL widget keeps render transform/opacity applying to the shadow too,
            // exactly as CSS transforms/fades an element's shadow with it.
            const auto& Margins = BakedShadows.OutsetMargins;
            Result = SNew(SOverlay)
                + SOverlay::Slot()
                    .Padding(FMargin(-Margins.Left, -Margins.Top, -Margins.Right, -Margins.Bottom))
                    [SNew(SImage).Image(BakedShadows.OutsetBrush)]
                + SOverlay::Slot()[Result];
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
    ApplyTextTransform(
        const FString& InContent,
        const FString& InTransformCase)
        -> FString
    {
        if (InTransformCase == TEXT("uppercase"))
        { return InContent.ToUpper(); }
        if (InTransformCase == TEXT("lowercase"))
        { return InContent.ToLower(); }
        if (InTransformCase == TEXT("capitalize"))
        {
            auto Result = InContent;
            auto AtWordStart = true;
            for (auto& Char : Result.GetCharArray())
            {
                if (AtWordStart)
                { Char = FChar::ToUpper(Char); }
                AtWordStart = FChar::IsWhitespace(Char);
            }
            return Result;
        }
        return InContent;
    }

    auto
    MakeWebFontInfo(
        const FCkWebUmg_IrText& InText)
        -> FSlateFontInfo
    {
        return ck_webumg_builder::MakeFontInfo(InText);
    }

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
