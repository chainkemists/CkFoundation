#include "CkSnapshot_SlotMeta.h"

#include "CkSnapshot/CkSnapshot_Log.h"

#include "Containers/Ticker.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HAL/FileManager.h" // deleting the HDR path's temp screenshot after reading it back

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

    namespace ck_snapshot_slotmeta
    {
        constexpr auto CaptureTimeoutSeconds = 2.0f;

        auto
            Encode_ScaledPng(
                const TArray<FColor>& InPixels,
                FIntPoint InSourceSize,
                int32 InMaxWidth)
            -> TArray<uint8>
        {
            if (InSourceSize.X <= 0 || InSourceSize.Y <= 0 || InPixels.Num() != InSourceSize.X * InSourceSize.Y)
            { return {}; }

            // The back buffer's alpha is scene-dependent and frequently 0; a thumbnail must be opaque
            // or it composites as an empty rect in the menu.
            auto OpaquePixels = InPixels;
            for (auto& Pixel : OpaquePixels)
            { Pixel.A = 255; }

            const auto TargetWidth = FMath::Clamp(InMaxWidth, 1, InSourceSize.X);
            const auto TargetHeight = FMath::Max(1,
                FMath::RoundToInt(static_cast<float>(InSourceSize.Y) * (static_cast<float>(TargetWidth) / static_cast<float>(InSourceSize.X))));

            auto ScaledPixels = TArray<FColor>{};

            constexpr auto ResizeInLinearSpace = true;
            FImageUtils::ImageResize(InSourceSize.X, InSourceSize.Y, OpaquePixels, TargetWidth, TargetHeight, ScaledPixels, ResizeInLinearSpace);

            auto CompressedPng = TArray64<uint8>{};
            FImageUtils::PNGCompressImageArray(TargetWidth, TargetHeight, TArrayView64<const FColor>{ScaledPixels.GetData(), ScaledPixels.Num()}, CompressedPng);

            auto Result = TArray<uint8>{};
            Result.Append(CompressedPng.GetData(), CompressedPng.Num());

            ck::snapshot::Verbose(TEXT("Thumbnail: encoded [{}x{}] -> [{}x{}], [{}] png bytes"),
                InSourceSize.X, InSourceSize.Y, TargetWidth, TargetHeight, Result.Num());

            return Result;
        }

        // Two engine delegates and a timeout ticker can each end the request; whichever runs first
        // completes and tears the others down, so the caller's callback fires exactly once.
        struct FPendingCapture
        {
            int32 _MaxWidth = 480;

            // Read at REQUEST time: ProcessScreenShots calls FScreenshotRequest::Reset() before
            // broadcasting OnScreenshotRequestProcessed, so the engine's copy is already cleared.
            FString _ScreenshotFilename;

            TFunction<void(TArray<uint8>)> _OnCaptured;
            FDelegateHandle _CapturedHandle;
            FDelegateHandle _ProcessedHandle;
            FTSTicker::FDelegateHandle _TimeoutHandle;
            bool _Completed = false;

            auto Complete(TArray<uint8> InPng) -> void
            {
                if (_Completed)
                { return; }
                _Completed = true;

                // Both are process-wide multicasts — a subscription left behind would eat a later,
                // unrelated screenshot.
                if (_CapturedHandle.IsValid())
                { UGameViewportClient::OnScreenshotCaptured().Remove(_CapturedHandle); }

                if (_ProcessedHandle.IsValid())
                { FScreenshotRequest::OnScreenshotRequestProcessed().Remove(_ProcessedHandle); }

                if (_TimeoutHandle.IsValid())
                { FTSTicker::GetCoreTicker().RemoveTicker(_TimeoutHandle); }

                if (_OnCaptured)
                { _OnCaptured(MoveTemp(InPng)); }
            }

            // ProcessScreenShots only broadcasts OnScreenshotCaptured for an LDR bitmap; an HDR
            // viewport writes an .exr instead and never fires it, so reading the file back is that
            // path's only channel. On LDR this lands after Complete() already ran, and no-ops.
            auto Complete_FromScreenshotFile() -> void
            {
                if (_Completed)
                { return; }

                auto Image = FImage{};

                if (_ScreenshotFilename.IsEmpty() || NOT FImageUtils::LoadImage(*_ScreenshotFilename, Image))
                {
                    ck::snapshot::Warning(TEXT("Request_CaptureViewportPng: the screenshot produced no in-memory bitmap "
                        "and no readable file at [{}] — no thumbnail captured."), _ScreenshotFilename);
                    Complete({});
                    return;
                }

                IFileManager::Get().Delete(*_ScreenshotFilename);

                Image.ChangeFormat(ERawImageFormat::BGRA8, Image.GammaSpace);
                const auto Pixels = TArray<FColor>{Image.AsBGRA8()};

                const auto SourceSize = FIntPoint{IntCastChecked<int32>(Image.GetWidth()), IntCastChecked<int32>(Image.GetHeight())};

                Complete(Encode_ScaledPng(Pixels, SourceSize, _MaxWidth));
            }
        };
    }

    auto
        Request_CaptureViewportPng(
            const UWorld* InWorld,
            int32 InMaxWidth,
            TFunction<void(TArray<uint8>)> InOnCaptured)
        -> void
    {
        const auto FireEmpty = [&InOnCaptured]() -> void
        {
            if (InOnCaptured)
            { InOnCaptured(TArray<uint8>{}); }
        };

        if (ck::Is_NOT_Valid(InWorld))
        { FireEmpty(); return; }

        const auto* ViewportClient = InWorld->GetGameViewport();

        if (ck::Is_NOT_Valid(ViewportClient) || ck::Is_NOT_Valid(ViewportClient->Viewport, ck::IsValid_Policy_NullptrOnly{}))
        { FireEmpty(); return; }

        if (ViewportClient->Viewport->GetSizeXY().X <= 0 || ViewportClient->Viewport->GetSizeXY().Y <= 0)
        { FireEmpty(); return; }

        auto Pending = MakeShared<ck_snapshot_slotmeta::FPendingCapture>();
        Pending->_MaxWidth = InMaxWidth;
        Pending->_OnCaptured = MoveTemp(InOnCaptured);

        // The LDR path: the engine hands us the bitmap in memory and writes no file.
        Pending->_CapturedHandle = UGameViewportClient::OnScreenshotCaptured().AddLambda(
            [Pending](int32 InWidth, int32 InHeight, const TArray<FColor>& InPixels) -> void
            {
                Pending->Complete(ck_snapshot_slotmeta::Encode_ScaledPng(InPixels, FIntPoint{InWidth, InHeight}, Pending->_MaxWidth));
            });

        // The HDR path (and r.ScreenshotDelegate 0). Bound unconditionally because HDR is a
        // display-side property we cannot ask the viewport about ahead of the capture.
        Pending->_ProcessedHandle = FScreenshotRequest::OnScreenshotRequestProcessed().AddLambda(
            [Pending]() -> void
            {
                Pending->Complete_FromScreenshotFile();
            });

        // The engine only services the request on a frame it actually renders. Without a bound wait the
        // caller would hang, so it is capped and answered with "no thumbnail" instead.
        Pending->_TimeoutHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([Pending](float) -> bool
            {
                ck::snapshot::Warning(TEXT("Request_CaptureViewportPng: no screenshot arrived within [{}]s "
                    "(is the viewport rendering?) — no thumbnail captured."),
                    ck_snapshot_slotmeta::CaptureTimeoutSeconds);
                Pending->Complete(TArray<uint8>{});
                return false;
            }), ck_snapshot_slotmeta::CaptureTimeoutSeconds);

        constexpr auto ShowUI = false;            // read the viewport before Slate composites UMG over it
        constexpr auto AddFilenameSuffix = true;  // unique name per request — the HDR path reads this file back
        FScreenshotRequest::RequestScreenshot(FString{}, ShowUI, AddFilenameSuffix);

        // AFTER the request: RequestScreenshot is what computes the name, and ProcessScreenShots resets
        // it before telling us the file is ready.
        Pending->_ScreenshotFilename = FScreenshotRequest::GetFilename();
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
