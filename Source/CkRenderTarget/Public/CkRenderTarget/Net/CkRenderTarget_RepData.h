#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkRenderTarget/RenderTarget/CkRenderTarget_Fragment_Data.h"

#include <GameplayTags.h>

#include "CkRenderTarget_RepData.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class APlayerState;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::render_target::transport
{
    // Fallbacks only — UCk_Utils_RenderTarget_Settings_UE is authoritative. Chunk size must sit
    // safely under the 64KB max-bunch ceiling.
    constexpr auto ChunkSizeBytes = 32768;
    constexpr auto MaxBytesPerStreamPerTick = 65536;
}

// --------------------------------------------------------------------------------------------------------------------

// One applied draw batch on the wire. _Sender is the authoring client, used for echo suppression
// (unset for server-authored batches); the relay stamps it server-side on the client-push path.
USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_RenderTarget_InstructionBatch
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RenderTarget_InstructionBatch);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame,
        meta = (AllowPrivateAccess = true))
    int32 _Seq = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame,
        meta = (AllowPrivateAccess = true))
    TArray<FCk_RenderTarget_DrawCmd> _Cmds;

    // Deliberately NOT SaveGame — runtime net identity, meaningless after a reload.
    UPROPERTY(VisibleAnywhere,
        meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<APlayerState> _Sender;

public:
    CK_PROPERTY(_Seq);
    CK_PROPERTY(_Cmds);
    CK_PROPERTY(_Sender);
};

// --------------------------------------------------------------------------------------------------------------------

// Per-sync-entity slice of the owner's replicated container, keyed by the sync child's _SyncName.
// A client that falls behind the ring's earliest seq has lost instructions and needs a pixel
// baseline (gap recovery).
USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_RenderTarget_ChannelState
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RenderTarget_ChannelState);

    static constexpr int32 RingSize = 64;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true, Categories = "RenderTarget"))
    FGameplayTag _SyncName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    TArray<FCk_RenderTarget_InstructionBatch> _Batches;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    int32 _LatestSeq = 0;

    // Bumped on every server-side pixel apply that is NOT instruction-driven (client uploads)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    int32 _PixelEpoch = 0;

public:
    CK_PROPERTY(_SyncName);
    CK_PROPERTY(_Batches);
    CK_PROPERTY(_LatestSeq);
    CK_PROPERTY(_PixelEpoch);
};

// --------------------------------------------------------------------------------------------------------------------

// The owner-entity replicated container payload: one channel per RenderTarget sync child.
USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_RepData_RenderTarget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RepData_RenderTarget);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    TArray<FCk_RenderTarget_ChannelState> _Channels;

public:
    CK_PROPERTY(_Channels);

public:
    auto
    Find_Channel(
        const FGameplayTag& InSyncName) -> FCk_RenderTarget_ChannelState*;

    auto
    Find_Channel(
        const FGameplayTag& InSyncName) const -> const FCk_RenderTarget_ChannelState*;
};

// --------------------------------------------------------------------------------------------------------------------

// Authoritative, snapshotable mirror of one sync child's applied instruction ring — it lives on the
// SYNC CHILD, not the owner. Rationale, recording sites, restore contract: CkRenderTarget/Claude.md.
USTRUCT()
struct CKRENDERTARGET_API FFragment_RenderTarget_AuthoredLog
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FFragment_RenderTarget_AuthoredLog);

private:
    UPROPERTY(SaveGame)
    TArray<FCk_RenderTarget_InstructionBatch> _Batches;

    UPROPERTY(SaveGame)
    int32 _NextBatchSeq = 1;

public:
    CK_PROPERTY_GET(_Batches);
    CK_PROPERTY_GET(_NextBatchSeq);

public:
    // Ring-capped to FCk_RenderTarget_ChannelState::RingSize; also advances the persisted author watermark.
    auto
    Record_PublishedBatch(
        const FCk_RenderTarget_InstructionBatch& InBatch,
        int32 InNextBatchSeq) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
