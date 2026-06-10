#include "CkRenderTarget_Readback.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkRenderTarget/CkRenderTarget_Log.h"
#include "CkRenderTarget/Pixels/CkRenderTarget_PixelMath.h"

#include <Engine/TextureRenderTarget2D.h>
#include <Misc/App.h>
#include <RHIGPUReadback.h>
#include <RenderingThread.h>
#include <Tasks/Task.h>
#include <TextureResource.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_RenderTarget_Readback::
    Request_EnqueueCopy(
        UTextureRenderTarget2D* InTarget)
    -> bool
{
    CK_ENSURE_IF_NOT(_State == ECk_RenderTarget_ReadbackState::Idle,
        TEXT("Request_EnqueueCopy called while a readback is already in state [{}]"),
        static_cast<int32>(_State))
    { return false; }

    if (NOT FApp::CanEverRender())
    { return false; }

    CK_ENSURE_IF_NOT(ck::IsValid(InTarget), TEXT("Request_EnqueueCopy called with an invalid render target"))
    { return false; }

    auto* Resource = InTarget->GameThread_GetRenderTargetResource();

    if (ck::Is_NOT_Valid(Resource, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    _Size = FIntPoint{InTarget->SizeX, InTarget->SizeY};
    _Readback = MakeShared<FRHIGPUTextureReadback>(TEXT("CkRenderTarget_Readback"));
    _Copy = MakeShared<FCopyState, ESPMode::ThreadSafe>();

    ENQUEUE_RENDER_COMMAND(CkRenderTarget_ReadbackEnqueue)(
        [Readback = _Readback, Copy = _Copy, Resource](FRHICommandListImmediate& RHICmdList) -> void
        {
            const auto& TextureRHI = Resource->GetRenderTargetTexture();

            if (NOT TextureRHI.IsValid())
            {
                Copy->_Failed = true;
                Copy->_EnqueueDone = true;
                return;
            }

            Readback->EnqueueCopy(RHICmdList, TextureRHI.GetReference());
            Copy->_EnqueueDone = true;
        });

    _State = ECk_RenderTarget_ReadbackState::AwaitingGpu;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_RenderTarget_Readback::
    Tick()
    -> ECk_RenderTarget_ReadbackState
{
    switch (_State)
    {
        case ECk_RenderTarget_ReadbackState::AwaitingGpu:
        {
            if (NOT _Copy->_EnqueueDone)
            { break; }

            if (_Copy->_Failed)
            {
                _State = ECk_RenderTarget_ReadbackState::Failed;
                break;
            }

            if (NOT _Readback->IsReady())
            { break; }

            ENQUEUE_RENDER_COMMAND(CkRenderTarget_ReadbackResolve)(
                [Readback = _Readback, Copy = _Copy, Size = _Size](FRHICommandListImmediate& RHICmdList) -> void
                {
                    auto RowPitchInPixels = 0;
                    auto* Data = Readback->Lock(RowPitchInPixels);

                    if (Data == nullptr)
                    {
                        Copy->_Failed = true;
                        Copy->_CopyDone = true;
                        return;
                    }

                    const auto RowBytes = Size.X * ck::render_target::pixel::BytesPerPixel;
                    const auto SrcStrideBytes = RowPitchInPixels * ck::render_target::pixel::BytesPerPixel;

                    Copy->_Pixels.SetNumUninitialized(RowBytes * Size.Y);

                    for (auto Row = 0; Row < Size.Y; ++Row)
                    {
                        FMemory::Memcpy(
                            Copy->_Pixels.GetData() + Row * RowBytes,
                            static_cast<const uint8*>(Data) + Row * SrcStrideBytes,
                            RowBytes);
                    }

                    Readback->Unlock();
                    Copy->_CopyDone = true;
                });

            _State = ECk_RenderTarget_ReadbackState::CopyInFlight;
            break;
        }
        case ECk_RenderTarget_ReadbackState::CopyInFlight:
        {
            if (NOT _Copy->_CopyDone)
            { break; }

            _State = _Copy->_Failed
                ? ECk_RenderTarget_ReadbackState::Failed
                : ECk_RenderTarget_ReadbackState::Complete;
            break;
        }
        default:
        { break; }
    }

    return _State;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_RenderTarget_Readback::
    TakePixels(
        TArray<uint8>& OutPixels,
        FIntPoint& OutSize)
    -> bool
{
    CK_ENSURE_IF_NOT(_State == ECk_RenderTarget_ReadbackState::Complete,
        TEXT("TakePixels called in state [{}] — only valid in Complete"),
        static_cast<int32>(_State))
    { return false; }

    OutPixels = MoveTemp(_Copy->_Pixels);
    OutSize = _Size;

    _Readback.Reset();
    _Copy.Reset();
    _State = ECk_RenderTarget_ReadbackState::Idle;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::render_target::pixel
{
    auto
    Launch_DiffAndCompressJob(
        TArray<uint8> InPixels,
        const FIntPoint& InSize,
        TArray<uint8> InPreviousSnapshot,
        const FIntPoint& InPreviousSize,
        int32 InBlockSize)
        -> TSharedPtr<FCk_RenderTarget_PixelJobResult, ESPMode::ThreadSafe>
    {
        auto Result = MakeShared<FCk_RenderTarget_PixelJobResult, ESPMode::ThreadSafe>();

        UE::Tasks::Launch(UE_SOURCE_LOCATION,
            [Result, Pixels = MoveTemp(InPixels), Size = InSize,
             PrevSnapshot = MoveTemp(InPreviousSnapshot), PrevSize = InPreviousSize, BlockSize = InBlockSize]() mutable -> void
            {
                const auto HasBaseline = PrevSnapshot.Num() > 0 && PrevSize == Size;

                if (NOT HasBaseline)
                {
                    Result->_Kind = ECk_RenderTarget_PixelPayloadKind::FullSync;
                    Result->_CompressedBytes = Compress(Pixels);
                    Result->_UncompressedSize = Pixels.Num();
                }
                else
                {
                    const auto Deltas = Diff_Blocks(Pixels, PrevSnapshot, Size, BlockSize);

                    if (Deltas.IsEmpty())
                    {
                        Result->_ZeroDiff = true;
                    }
                    else
                    {
                        const auto Serialized = Serialize_Deltas(Deltas);
                        Result->_Kind = ECk_RenderTarget_PixelPayloadKind::Delta;
                        Result->_CompressedBytes = Compress(Serialized);
                        Result->_UncompressedSize = Serialized.Num();
                    }
                }

                Result->_NewSnapshot = MoveTemp(Pixels);
                Result->_Size = Size;
                Result->_Done = true;
            });

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Launch_DecompressAndApplyJob(
        ECk_RenderTarget_PixelPayloadKind InKind,
        TArray<uint8> InCompressed,
        int32 InUncompressedSize,
        const FIntPoint& InSize,
        TArray<uint8> InPreviousStaging,
        const FIntPoint& InPreviousSize,
        int32 InBlockSize)
        -> TSharedPtr<FCk_RenderTarget_PixelApplyJobResult, ESPMode::ThreadSafe>
    {
        auto Result = MakeShared<FCk_RenderTarget_PixelApplyJobResult, ESPMode::ThreadSafe>();

        UE::Tasks::Launch(UE_SOURCE_LOCATION,
            [Result, Kind = InKind, Compressed = MoveTemp(InCompressed), UncompressedSize = InUncompressedSize,
             Size = InSize, PrevStaging = MoveTemp(InPreviousStaging), PrevSize = InPreviousSize,
             BlockSize = InBlockSize]() mutable -> void
            {
                auto Raw = Decompress(Compressed, UncompressedSize);

                if (NOT Raw.IsSet())
                {
                    Result->_Failed = true;
                    Result->_Done = true;
                    return;
                }

                switch (Kind)
                {
                    case ECk_RenderTarget_PixelPayloadKind::FullSync:
                    {
                        if (Raw->Num() != Size.X * Size.Y * BytesPerPixel)
                        {
                            Result->_Failed = true;
                            break;
                        }

                        Result->_NewStaging = MoveTemp(*Raw);
                        Result->_Size = Size;
                        break;
                    }
                    case ECk_RenderTarget_PixelPayloadKind::Delta:
                    {
                        if (PrevStaging.Num() != Size.X * Size.Y * BytesPerPixel || PrevSize != Size)
                        {
                            Result->_Failed = true;
                            break;
                        }

                        const auto Deltas = Deserialize_Deltas(*Raw);

                        if (NOT Deltas.IsSet())
                        {
                            Result->_Failed = true;
                            break;
                        }

                        Patch_Blocks(PrevStaging, Size, *Deltas, BlockSize);
                        Result->_NewStaging = MoveTemp(PrevStaging);
                        Result->_Size = Size;
                        break;
                    }
                }

                Result->_Done = true;
            });

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
