#include "CkSnapshot_SlotMeta.h"

#include "CkSnapshot/CkSnapshot_Log.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

#include "ImageUtils.h"
#include "UnrealClient.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Snapshot_SlotMeta::
    Get_IsPopulated() const
    -> bool
{
    return _TimestampUTC != FDateTime{};
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot::slot_meta
{
    const FString MetaSlotSuffix = TEXT(".meta");

    auto
        Get_MetaSlotName(
            FName InSlotName)
        -> FString
    {
        return InSlotName.ToString() + MetaSlotSuffix;
    }

    auto
        Get_IsMetaSlotName(
            const FString& InSlotName)
        -> bool
    {
        return InSlotName.EndsWith(MetaSlotSuffix, ESearchCase::CaseSensitive);
    }

    auto
        Capture_ViewportPng(
            const UWorld* InWorld,
            int32 InMaxWidth)
        -> TArray<uint8>
    {
        if (ck::Is_NOT_Valid(InWorld))
        { return {}; }

        const auto* ViewportClient = InWorld->GetGameViewport();

        if (ck::Is_NOT_Valid(ViewportClient) || ck::Is_NOT_Valid(ViewportClient->Viewport, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        auto* Viewport = ViewportClient->Viewport;
        const auto SourceSize = Viewport->GetSizeXY();

        if (SourceSize.X <= 0 || SourceSize.Y <= 0)
        { return {}; }

        auto SourcePixels = TArray<FColor>{};

        if (NOT Viewport->ReadPixels(SourcePixels,
                FReadSurfaceDataFlags{RCM_UNorm, CubeFace_MAX},
                FIntRect{0, 0, SourceSize.X, SourceSize.Y}))
        {
            ck::snapshot::Warning(TEXT("Capture_ViewportPng: ReadPixels failed on a [{}x{}] viewport"),
                SourceSize.X, SourceSize.Y);
            return {};
        }

        if (SourcePixels.Num() != SourceSize.X * SourceSize.Y)
        { return {}; }

        // The back buffer's alpha is scene-dependent and frequently 0; a thumbnail must be opaque or
        // it composites as an empty rect in the menu.
        for (auto& Pixel : SourcePixels)
        { Pixel.A = 255; }

        const auto TargetWidth = FMath::Clamp(InMaxWidth, 1, SourceSize.X);
        const auto TargetHeight = FMath::Max(1,
            FMath::RoundToInt(static_cast<float>(SourceSize.Y) * (static_cast<float>(TargetWidth) / static_cast<float>(SourceSize.X))));

        auto ScaledPixels = TArray<FColor>{};

        constexpr auto ResizeInLinearSpace = true;
        FImageUtils::ImageResize(SourceSize.X, SourceSize.Y, SourcePixels, TargetWidth, TargetHeight, ScaledPixels, ResizeInLinearSpace);

        auto CompressedPng = TArray64<uint8>{};
        FImageUtils::PNGCompressImageArray(TargetWidth, TargetHeight, TArrayView64<const FColor>{ScaledPixels.GetData(), ScaledPixels.Num()}, CompressedPng);

        auto Result = TArray<uint8>{};
        Result.Append(CompressedPng.GetData(), CompressedPng.Num());

        ck::snapshot::Verbose(TEXT("Capture_ViewportPng: captured [{}x{}] -> [{}x{}], [{}] png bytes"),
            SourceSize.X, SourceSize.Y, TargetWidth, TargetHeight, Result.Num());

        return Result;
    }

    auto
        Decode_PngAsTexture(
            const TArray<uint8>& InPng)
        -> UTexture2D*
    {
        if (InPng.IsEmpty())
        { return nullptr; }

        return FImageUtils::ImportBufferAsTexture2D(InPng);
    }
}

// --------------------------------------------------------------------------------------------------------------------
