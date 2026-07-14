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
    // Fallback defaults for when the project-settings CDO is unavailable —
    // UCk_Utils_RenderTarget_Settings_UE is the authoritative source. Chunk size must sit safely
    // under the 64KB max-bunch ceiling.
    constexpr auto ChunkSizeBytes = 32768;
    constexpr auto MaxBytesPerStreamPerTick = 65536;
}

// --------------------------------------------------------------------------------------------------------------------

// One applied draw batch on the wire. _Sender identifies the authoring client for echo
// suppression (unset for server-authored batches); the relay stamps it server-side on the
// client-push path.
USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_RenderTarget_InstructionBatch
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RenderTarget_InstructionBatch);

private:
    // SaveGame: persisted by CkSnapshot (FFragment_RenderTarget_AuthoredLog snapshots the published
    // batch ring; the restored host re-applies + re-publishes these via HydrateFromSavedChannel on load).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame,
        meta = (AllowPrivateAccess = true))
    int32 _Seq = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame,
        meta = (AllowPrivateAccess = true))
    TArray<FCk_RenderTarget_DrawCmd> _Cmds;

    // NOT SaveGame: _Sender is runtime net identity (the authoring PlayerState) — meaningless after a
    // reload/seamless travel. Dropped on restore, so re-published batches read as server-authored.
    UPROPERTY(VisibleAnywhere,
        meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<APlayerState> _Sender;

public:
    CK_PROPERTY(_Seq);
    CK_PROPERTY(_Cmds);
    CK_PROPERTY(_Sender);
};

// --------------------------------------------------------------------------------------------------------------------

// Per-sync-entity slice of the owner's replicated container — keyed by the sync child's
// _SyncName label. The batch ring holds the last RingSize applied batches; clients that fall
// behind the ring's earliest seq have lost instructions and need a pixel baseline (gap recovery).
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

    // Bumped on every server-side pixel apply that is NOT instruction-driven (client uploads) —
    // tells receivers a pixel reconcile is coming
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

// The owner-entity replicated container payload: one channel per RenderTarget sync child on the
// owner. Attached on authority by FProcessor_RenderTarget_Setup; applied on clients by the
// RegisterLazy handler in CkRenderTarget_Replication.cpp.
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

// Authoritative, snapshotable mirror of one sync child's applied instruction ring. Lives on the SYNC
// CHILD entity (not the owner): the owner-hosted FCk_RepData_RenderTarget container is stored inside
// the replication driver's FastArray, which is re-created EMPTY on a snapshot load and is not itself a
// snapshotable ECS fragment — so the instruction stream has no other persistent home.
//
// Recording sites (both ring-capped to RingSize identically to the channel):
//   - Replicates + non-Pixels host: FProcessor_RenderTarget_HandleRequests::DoPublishBatch (covers
//     direct server draws AND applied client batches).
//   - DoesNotReplicate (any mode):  FProcessor_RenderTarget_HandleRequests::ForEachEntity — local-only
//     targets never publish but their drawn state must still persist.
//   - Replicates + Pixels: NOT recorded — pixel-baseline persistence is the documented v1 deferral.
//
// Setup additionally AddOrGets an EMPTY log on every sync entity: it is the composition anchor for the
// snapshot-restore view (Params + AuthoredLog), so a never-drawn target still restores fully composed.
// HydrateFromSavedChannel reads the log back after a load to repaint the restored target and
// (Replicates only) re-publish into a fresh owner container.
USTRUCT()
struct CKRENDERTARGET_API FFragment_RenderTarget_AuthoredLog
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FFragment_RenderTarget_AuthoredLog);

    // Tier-A snapshotable: pure value data (no entity-handle refs), captured via tagged-property
    // serialization of the SaveGame fields below.
    using IsSnapshotable = void;

private:
    UPROPERTY(SaveGame)
    TArray<FCk_RenderTarget_InstructionBatch> _Batches;

    UPROPERTY(SaveGame)
    int32 _NextBatchSeq = 1;

public:
    CK_PROPERTY_GET(_Batches);
    CK_PROPERTY_GET(_NextBatchSeq);

public:
    // Records a just-applied/published batch (ring-capped to FCk_RenderTarget_ChannelState::RingSize)
    // and advances the persisted author watermark. See the recording-sites note above.
    auto
    Record_PublishedBatch(
        const FCk_RenderTarget_InstructionBatch& InBatch,
        int32 InNextBatchSeq) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
