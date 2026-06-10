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
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    int32 _Seq = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    TArray<FCk_RenderTarget_DrawCmd> _Cmds;

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
