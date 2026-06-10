#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkRenderTarget/RenderTarget/CkRenderTarget_Fragment_Data.h"

#include <Containers/Array.h>
#include <Math/IntPoint.h>
#include <Templates/SharedPointer.h>

#include <atomic>

// --------------------------------------------------------------------------------------------------------------------

class FRHIGPUTextureReadback;
class UTextureRenderTarget2D;

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_RenderTarget_ReadbackState : uint8
{
    Idle,
    // Copy command enqueued, waiting for the GPU fence
    AwaitingGpu,
    // Fence signaled, render-thread lock+copy into the CPU buffer is in flight
    CopyInFlight,
    // CPU buffer is populated — TakePixels may be called
    Complete,
    Failed
};

// --------------------------------------------------------------------------------------------------------------------

// Non-blocking GPU readback of an RGBA8 render target. Lifecycle (all game-thread except where
// noted): Request_EnqueueCopy → Tick() until Complete/Failed → TakePixels. Never flushes
// rendering commands; the fence is polled and the staging-lock copy runs on the render thread.
class CKRENDERTARGET_API FCk_RenderTarget_Readback
{
public:
    CK_GENERATED_BODY(FCk_RenderTarget_Readback);

public:
    // Returns false (and stays Idle) when the process cannot render or the target has no
    // usable resource — callers should treat that as "pixel capture unavailable".
    auto
    Request_EnqueueCopy(
        UTextureRenderTarget2D* InTarget) -> bool;

    // Advances AwaitingGpu → CopyInFlight when the fence is ready, and CopyInFlight →
    // Complete/Failed when the render-thread copy lands. Returns the post-advance state.
    auto
    Tick() -> ECk_RenderTarget_ReadbackState;

    // Moves the copied pixels out (valid once in Complete; resets to Idle).
    auto
    TakePixels(
        TArray<uint8>& OutPixels,
        FIntPoint& OutSize) -> bool;

public:
    CK_PROPERTY_GET(_State);

private:
    struct FCopyState
    {
        std::atomic<bool> _EnqueueDone{false};
        std::atomic<bool> _CopyDone{false};
        std::atomic<bool> _Failed{false};
        TArray<uint8> _Pixels;
    };

private:
    TSharedPtr<FRHIGPUTextureReadback> _Readback;
    TSharedPtr<FCopyState, ESPMode::ThreadSafe> _Copy;
    FIntPoint _Size = FIntPoint::ZeroValue;
    ECk_RenderTarget_ReadbackState _State = ECk_RenderTarget_ReadbackState::Idle;
};

// --------------------------------------------------------------------------------------------------------------------

// Output of one background diff+compress pass. Owned via thread-safe shared ptr — the game
// thread polls _Done while the UE::Tasks worker fills the rest.
struct CKRENDERTARGET_API FCk_RenderTarget_PixelJobResult
{
    std::atomic<bool> _Done{false};

    // Identical to the previous snapshot — nothing to send, snapshot unchanged
    bool _ZeroDiff = false;

    ECk_RenderTarget_PixelPayloadKind _Kind = ECk_RenderTarget_PixelPayloadKind::FullSync;
    TArray<uint8> _CompressedBytes;
    int32 _UncompressedSize = 0;

    // The captured pixels — becomes the new _LastSyncedSnapshot
    TArray<uint8> _NewSnapshot;
    FIntPoint _Size = FIntPoint::ZeroValue;
};

// --------------------------------------------------------------------------------------------------------------------

// Output of one receive-side decompress+apply pass: the receiver's new staging buffer (FullSync:
// the decompressed image; Delta: previous staging patched with the decoded blocks).
struct CKRENDERTARGET_API FCk_RenderTarget_PixelApplyJobResult
{
    std::atomic<bool> _Done{false};
    bool _Failed = false;
    TArray<uint8> _NewStaging;
    FIntPoint _Size = FIntPoint::ZeroValue;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::render_target::pixel
{
    // Kicks the diff+compress background job: no previous snapshot (or size change) → FullSync
    // payload; otherwise block-diff vs the snapshot → Delta payload, or ZeroDiff when identical.
    CKRENDERTARGET_API auto
    Launch_DiffAndCompressJob(
        TArray<uint8> InPixels,
        const FIntPoint& InSize,
        TArray<uint8> InPreviousSnapshot,
        const FIntPoint& InPreviousSize,
        int32 InBlockSize) -> TSharedPtr<FCk_RenderTarget_PixelJobResult, ESPMode::ThreadSafe>;

    // Receive-side inverse: decompress a payload and produce the new staging buffer. Delta
    // payloads fail when the previous staging is absent or size-mismatched (no baseline).
    CKRENDERTARGET_API auto
    Launch_DecompressAndApplyJob(
        ECk_RenderTarget_PixelPayloadKind InKind,
        TArray<uint8> InCompressed,
        int32 InUncompressedSize,
        const FIntPoint& InSize,
        TArray<uint8> InPreviousStaging,
        const FIntPoint& InPreviousSize,
        int32 InBlockSize) -> TSharedPtr<FCk_RenderTarget_PixelApplyJobResult, ESPMode::ThreadSafe>;
}

// --------------------------------------------------------------------------------------------------------------------
