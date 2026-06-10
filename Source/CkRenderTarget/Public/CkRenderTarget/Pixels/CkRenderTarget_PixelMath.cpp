#include "CkRenderTarget_PixelMath.h"

#include "CkCore/Ensure/CkEnsure.h"

#include <Misc/Compression.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_render_target_pixel_math
{
    // Copies one block's rect between a tightly-packed block buffer and the full image.
    // Direction is expressed by which buffer is the destination.
    auto
    CopyBlockRows(
        uint8* InDst,
        int32 InDstStrideBytes,
        const uint8* InSrc,
        int32 InSrcStrideBytes,
        int32 InRowBytes,
        int32 InNumRows) -> void
    {
        for (auto Row = 0; Row < InNumRows; ++Row)
        {
            FMemory::Memcpy(InDst + Row * InDstStrideBytes, InSrc + Row * InSrcStrideBytes, InRowBytes);
        }
    }

    auto
    Get_BlockRect(
        const FIntPoint& InSize,
        int32 InBlockSize,
        int32 InBlockX,
        int32 InBlockY) -> FIntRect
    {
        const auto MinX = InBlockX * InBlockSize;
        const auto MinY = InBlockY * InBlockSize;
        return FIntRect
        {
            FIntPoint{MinX, MinY},
            FIntPoint{FMath::Min(MinX + InBlockSize, InSize.X), FMath::Min(MinY + InBlockSize, InSize.Y)}
        };
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::render_target::pixel
{
    auto
    Get_BlockCounts(
        const FIntPoint& InSize,
        int32 InBlockSize)
        -> FIntPoint
    {
        CK_ENSURE_IF_NOT(InBlockSize > 0, TEXT("Invalid block size [{}]"), InBlockSize)
        { return FIntPoint::ZeroValue; }

        return FIntPoint
        {
            FMath::DivideAndRoundUp(InSize.X, InBlockSize),
            FMath::DivideAndRoundUp(InSize.Y, InBlockSize)
        };
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Diff_Blocks(
        const TArray<uint8>& InCurrent,
        const TArray<uint8>& InPrevious,
        const FIntPoint& InSize,
        int32 InBlockSize)
        -> TArray<FCk_RenderTarget_BlockDelta>
    {
        const auto ExpectedBytes = InSize.X * InSize.Y * BytesPerPixel;

        CK_ENSURE_IF_NOT(InCurrent.Num() == ExpectedBytes && InPrevious.Num() == ExpectedBytes,
            TEXT("Diff_Blocks buffer size mismatch — current [{}], previous [{}], expected [{}] for [{}x{}]"),
            InCurrent.Num(), InPrevious.Num(), ExpectedBytes, InSize.X, InSize.Y)
        { return {}; }

        const auto BlockCounts = Get_BlockCounts(InSize, InBlockSize);
        const auto ImageStrideBytes = InSize.X * BytesPerPixel;

        auto Deltas = TArray<FCk_RenderTarget_BlockDelta>{};

        for (auto BlockY = 0; BlockY < BlockCounts.Y; ++BlockY)
        {
            for (auto BlockX = 0; BlockX < BlockCounts.X; ++BlockX)
            {
                const auto Rect = ck_render_target_pixel_math::Get_BlockRect(InSize, InBlockSize, BlockX, BlockY);
                const auto RowBytes = Rect.Width() * BytesPerPixel;
                const auto FirstByte = Rect.Min.Y * ImageStrideBytes + Rect.Min.X * BytesPerPixel;

                auto RowsDiffer = false;
                for (auto Row = 0; Row < Rect.Height(); ++Row)
                {
                    const auto Offset = FirstByte + Row * ImageStrideBytes;
                    if (FMemory::Memcmp(InCurrent.GetData() + Offset, InPrevious.GetData() + Offset, RowBytes) != 0)
                    {
                        RowsDiffer = true;
                        break;
                    }
                }

                if (NOT RowsDiffer)
                { continue; }

                auto BlockPixels = TArray<uint8>{};
                BlockPixels.SetNumUninitialized(RowBytes * Rect.Height());
                ck_render_target_pixel_math::CopyBlockRows(
                    BlockPixels.GetData(), RowBytes,
                    InCurrent.GetData() + FirstByte, ImageStrideBytes,
                    RowBytes, Rect.Height());

                Deltas.Emplace(BlockX, BlockY, MoveTemp(BlockPixels));
            }
        }

        return Deltas;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Patch_Blocks(
        TArray<uint8>& InOutPixels,
        const FIntPoint& InSize,
        const TArray<FCk_RenderTarget_BlockDelta>& InDeltas,
        int32 InBlockSize)
        -> void
    {
        const auto ExpectedBytes = InSize.X * InSize.Y * BytesPerPixel;

        CK_ENSURE_IF_NOT(InOutPixels.Num() == ExpectedBytes,
            TEXT("Patch_Blocks buffer size mismatch — got [{}], expected [{}] for [{}x{}]"),
            InOutPixels.Num(), ExpectedBytes, InSize.X, InSize.Y)
        { return; }

        const auto BlockCounts = Get_BlockCounts(InSize, InBlockSize);
        const auto ImageStrideBytes = InSize.X * BytesPerPixel;

        for (const auto& Delta : InDeltas)
        {
            CK_ENSURE_IF_NOT(Delta.Get_BlockX() >= 0 && Delta.Get_BlockX() < BlockCounts.X &&
                             Delta.Get_BlockY() >= 0 && Delta.Get_BlockY() < BlockCounts.Y,
                TEXT("Patch_Blocks delta block [{}, {}] is outside the [{}x{}] block grid — skipping"),
                Delta.Get_BlockX(), Delta.Get_BlockY(), BlockCounts.X, BlockCounts.Y)
            { continue; }

            const auto Rect = ck_render_target_pixel_math::Get_BlockRect(InSize, InBlockSize, Delta.Get_BlockX(), Delta.Get_BlockY());
            const auto RowBytes = Rect.Width() * BytesPerPixel;
            const auto FirstByte = Rect.Min.Y * ImageStrideBytes + Rect.Min.X * BytesPerPixel;

            CK_ENSURE_IF_NOT(Delta.Get_Pixels().Num() == RowBytes * Rect.Height(),
                TEXT("Patch_Blocks delta block [{}, {}] content size [{}] does not match its rect [{}x{}] — skipping"),
                Delta.Get_BlockX(), Delta.Get_BlockY(), Delta.Get_Pixels().Num(), Rect.Width(), Rect.Height())
            { continue; }

            ck_render_target_pixel_math::CopyBlockRows(
                InOutPixels.GetData() + FirstByte, ImageStrideBytes,
                Delta.Get_Pixels().GetData(), RowBytes,
                RowBytes, Rect.Height());
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Serialize_Deltas(
        const TArray<FCk_RenderTarget_BlockDelta>& InDeltas)
        -> TArray<uint8>
    {
        auto TotalBytes = static_cast<int32>(sizeof(int32));
        for (const auto& Delta : InDeltas)
        {
            TotalBytes += static_cast<int32>(sizeof(int32)) * 3 + Delta.Get_Pixels().Num();
        }

        auto Buffer = TArray<uint8>{};
        Buffer.Reserve(TotalBytes);

        const auto AppendInt32 = [&](int32 InValue) -> void
        {
            Buffer.Append(reinterpret_cast<const uint8*>(&InValue), sizeof(int32));
        };

        AppendInt32(InDeltas.Num());
        for (const auto& Delta : InDeltas)
        {
            AppendInt32(Delta.Get_BlockX());
            AppendInt32(Delta.Get_BlockY());
            AppendInt32(Delta.Get_Pixels().Num());
            Buffer.Append(Delta.Get_Pixels());
        }

        return Buffer;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Deserialize_Deltas(
        const TArray<uint8>& InBuffer)
        -> TOptional<TArray<FCk_RenderTarget_BlockDelta>>
    {
        auto Cursor = 0;

        const auto ReadInt32 = [&](int32& OutValue) -> bool
        {
            if (Cursor + static_cast<int32>(sizeof(int32)) > InBuffer.Num())
            { return false; }

            FMemory::Memcpy(&OutValue, InBuffer.GetData() + Cursor, sizeof(int32));
            Cursor += sizeof(int32);
            return true;
        };

        auto Count = 0;
        if (NOT ReadInt32(Count) || Count < 0)
        { return {}; }

        auto Deltas = TArray<FCk_RenderTarget_BlockDelta>{};
        Deltas.Reserve(Count);

        for (auto Index = 0; Index < Count; ++Index)
        {
            auto BlockX = 0;
            auto BlockY = 0;
            auto NumBytes = 0;

            if (NOT ReadInt32(BlockX) || NOT ReadInt32(BlockY) || NOT ReadInt32(NumBytes))
            { return {}; }

            if (NumBytes < 0 || Cursor + NumBytes > InBuffer.Num())
            { return {}; }

            auto Pixels = TArray<uint8>{};
            Pixels.SetNumUninitialized(NumBytes);
            FMemory::Memcpy(Pixels.GetData(), InBuffer.GetData() + Cursor, NumBytes);
            Cursor += NumBytes;

            Deltas.Emplace(BlockX, BlockY, MoveTemp(Pixels));
        }

        if (Cursor != InBuffer.Num())
        { return {}; }

        return Deltas;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Chunk_Payload(
        const TArray<uint8>& InPayload,
        int32 InChunkSizeBytes)
        -> TArray<TArray<uint8>>
    {
        CK_ENSURE_IF_NOT(InChunkSizeBytes > 0, TEXT("Invalid chunk size [{}]"), InChunkSizeBytes)
        { return {}; }

        auto Chunks = TArray<TArray<uint8>>{};
        Chunks.Reserve(FMath::DivideAndRoundUp(InPayload.Num(), InChunkSizeBytes));

        for (auto Cursor = 0; Cursor < InPayload.Num(); Cursor += InChunkSizeBytes)
        {
            const auto ChunkBytes = FMath::Min(InChunkSizeBytes, InPayload.Num() - Cursor);

            auto Chunk = TArray<uint8>{};
            Chunk.SetNumUninitialized(ChunkBytes);
            FMemory::Memcpy(Chunk.GetData(), InPayload.GetData() + Cursor, ChunkBytes);
            Chunks.Emplace(MoveTemp(Chunk));
        }

        return Chunks;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Reassemble_Chunks(
        const TArray<TArray<uint8>>& InChunks)
        -> TArray<uint8>
    {
        auto TotalBytes = 0;
        for (const auto& Chunk : InChunks)
        {
            TotalBytes += Chunk.Num();
        }

        auto Payload = TArray<uint8>{};
        Payload.Reserve(TotalBytes);

        for (const auto& Chunk : InChunks)
        {
            Payload.Append(Chunk);
        }

        return Payload;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Compress(
        const TArray<uint8>& InRaw)
        -> TArray<uint8>
    {
        if (InRaw.IsEmpty())
        { return {}; }

        auto CompressedSize = FCompression::CompressMemoryBound(NAME_Oodle, InRaw.Num());

        auto Compressed = TArray<uint8>{};
        Compressed.SetNumUninitialized(CompressedSize);

        const auto DidCompress = FCompression::CompressMemory(
            NAME_Oodle, Compressed.GetData(), CompressedSize, InRaw.GetData(), InRaw.Num());

        CK_ENSURE_IF_NOT(DidCompress, TEXT("Oodle compression failed for a [{}] byte payload"), InRaw.Num())
        { return {}; }

        Compressed.SetNum(CompressedSize);
        return Compressed;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Decompress(
        const TArray<uint8>& InCompressed,
        int32 InUncompressedSize)
        -> TOptional<TArray<uint8>>
    {
        if (InUncompressedSize == 0 && InCompressed.IsEmpty())
        { return TArray<uint8>{}; }

        if (InUncompressedSize <= 0 || InCompressed.IsEmpty())
        { return {}; }

        auto Raw = TArray<uint8>{};
        Raw.SetNumUninitialized(InUncompressedSize);

        const auto DidDecompress = FCompression::UncompressMemory(
            NAME_Oodle, Raw.GetData(), InUncompressedSize, InCompressed.GetData(), InCompressed.Num());

        if (NOT DidDecompress)
        { return {}; }

        return Raw;
    }
}

// --------------------------------------------------------------------------------------------------------------------
