#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Containers/Array.h>
#include <Math/IntPoint.h>
#include <Misc/Optional.h>

// --------------------------------------------------------------------------------------------------------------------

// One changed block of a block-diff. _Pixels holds the block's content tightly packed
// (row-major, BytesPerPixel per pixel, rows clamped to the image edge for edge blocks).
struct CKRENDERTARGET_API FCk_RenderTarget_BlockDelta
{
public:
    CK_GENERATED_BODY(FCk_RenderTarget_BlockDelta);

private:
    int32 _BlockX = 0;
    int32 _BlockY = 0;
    TArray<uint8> _Pixels;

public:
    CK_PROPERTY_GET(_BlockX);
    CK_PROPERTY_GET(_BlockY);
    CK_PROPERTY_GET(_Pixels);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_RenderTarget_BlockDelta, _BlockX, _BlockY, _Pixels);
};

// --------------------------------------------------------------------------------------------------------------------

// Pure pixel/payload math — no UObjects, no ECS, fully unit-testable. All buffers are raw
// RGBA8 (4 bytes/pixel, row-major, tightly packed).
namespace ck::render_target::pixel
{
    constexpr auto BytesPerPixel = 4;

    // Fallback for when the BlockSize project setting is unavailable
    constexpr auto DefaultBlockSize = 16;

    // Number of blocks per axis for an image of InSize at InBlockSize (edge blocks count even
    // when smaller than InBlockSize).
    CKRENDERTARGET_API auto
    Get_BlockCounts(
        const FIntPoint& InSize,
        int32 InBlockSize) -> FIntPoint;

    // Block-level diff of two same-sized images. Returns one delta per block whose content
    // differs. Empty result == identical images.
    CKRENDERTARGET_API auto
    Diff_Blocks(
        const TArray<uint8>& InCurrent,
        const TArray<uint8>& InPrevious,
        const FIntPoint& InSize,
        int32 InBlockSize) -> TArray<FCk_RenderTarget_BlockDelta>;

    // Applies block deltas onto InOutPixels in place. Deltas outside the image or with
    // mismatched content size are skipped with an ensure.
    CKRENDERTARGET_API auto
    Patch_Blocks(
        TArray<uint8>& InOutPixels,
        const FIntPoint& InSize,
        const TArray<FCk_RenderTarget_BlockDelta>& InDeltas,
        int32 InBlockSize) -> void;

    // Flat wire form of a delta set: [int32 Count] then per delta [int32 BlockX][int32 BlockY]
    // [int32 NumBytes][bytes]. Deserialize returns unset on any malformed/truncated input.
    CKRENDERTARGET_API auto
    Serialize_Deltas(
        const TArray<FCk_RenderTarget_BlockDelta>& InDeltas) -> TArray<uint8>;

    CKRENDERTARGET_API auto
    Deserialize_Deltas(
        const TArray<uint8>& InBuffer) -> TOptional<TArray<FCk_RenderTarget_BlockDelta>>;

    // Splits a payload into <= InChunkSizeBytes pieces (last chunk may be smaller; empty
    // payload yields no chunks). Reassemble is the exact inverse.
    CKRENDERTARGET_API auto
    Chunk_Payload(
        const TArray<uint8>& InPayload,
        int32 InChunkSizeBytes) -> TArray<TArray<uint8>>;

    CKRENDERTARGET_API auto
    Reassemble_Chunks(
        const TArray<TArray<uint8>>& InChunks) -> TArray<uint8>;

    // Oodle round-trip. Decompress returns unset when the buffer does not decompress to
    // exactly InUncompressedSize.
    CKRENDERTARGET_API auto
    Compress(
        const TArray<uint8>& InRaw) -> TArray<uint8>;

    CKRENDERTARGET_API auto
    Decompress(
        const TArray<uint8>& InCompressed,
        int32 InUncompressedSize) -> TOptional<TArray<uint8>>;
}

// --------------------------------------------------------------------------------------------------------------------
