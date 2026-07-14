#pragma once

#include "CkStateMachine/Net/CkStateMachine_NetContext.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkStateMachine_RepData.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Reflected save-only mirror of ck::FFragment_Sm_StateOverrides::FEntry, carried inside both SM RepData
// shapes so runtime-installed state overrides survive save/load. Fields mirror FEntry exactly:
// _OverrideStateClass is the class swapped in, _CachedStatesToOverride are the state tags it overrides
// (cached from the override class CDO's Get_StatesToOverride() at add time). Not on the wire — the net
// receive path never reads it; only Produce fills it and only HydrationApply consumes it.
USTRUCT()
struct CKSTATEMACHINE_API FCk_Sm_SavedStateOverride
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sm_SavedStateOverride);

private:
    UPROPERTY()
    TSubclassOf<UCk_SmState_EntityScript> _OverrideStateClass;

    UPROPERTY()
    TArray<FGameplayTag> _CachedStatesToOverride;

public:
    CK_PROPERTY(_OverrideStateClass);
    CK_PROPERTY(_CachedStatesToOverride);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sm_SavedStateOverride, _OverrideStateClass, _CachedStatesToOverride);
};

// --------------------------------------------------------------------------------------------------------------------

// Replication payload for SMs in WithHistory mode. Carries a rolling window of transition events
// (size 64), the current run status, and the initial-state fingerprint. NOTE: the spec §9.5
// Setup-time check that would consume _InitialStateFingerprint was never implemented — the field
// is stamped by the publisher backfill and is currently informational (read only by test support).
USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_RepData_StateMachine_WithHistory
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RepData_StateMachine_WithHistory);

    static constexpr int32 RingSize = 64;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    TArray<FCk_Sm_TransitionEvent> _History;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    ECk_SmRunStatus _RunStatus = ECk_SmRunStatus::Stopped;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    int32 _InitialStateFingerprint = 0;

    // Save-only: filled by the save-transport Produce handler from the entity's
    // ck::FFragment_Sm_StateOverrides, consumed by HydrationApply on load. NEVER filled by the wire
    // publish paths (the TryUpdateContainerFragment sites in DoPublishRunStatus /
    // FProcessor_Sm_CommitPendingTransition) — stays an empty array on every replicated delta.
    UPROPERTY()
    TArray<FCk_Sm_SavedStateOverride> _SavedStateOverrides;

public:
    CK_PROPERTY(_History);
    CK_PROPERTY(_RunStatus);
    CK_PROPERTY(_InitialStateFingerprint);
    CK_PROPERTY(_SavedStateOverrides);
};

// --------------------------------------------------------------------------------------------------------------------

// Replication payload for SMs in WithoutHistory mode. Carries the latest state only — no replay,
// non-owning clients snap forward.
USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_RepData_StateMachine_NoHistory
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RepData_StateMachine_NoHistory);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_SmState_EntityScript> _CurrentStateClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    int32 _Seq = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    int32 _CurrentStateFingerprint = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    ECk_SmRunStatus _RunStatus = ECk_SmRunStatus::Stopped;

    // Save-only: filled by the save-transport Produce handler from the entity's
    // ck::FFragment_Sm_StateOverrides, consumed by HydrationApply on load. NEVER filled by the wire
    // publish paths (the TryUpdateContainerFragment sites in DoPublishRunStatus /
    // FProcessor_Sm_CommitPendingTransition) — stays an empty array on every replicated delta.
    UPROPERTY()
    TArray<FCk_Sm_SavedStateOverride> _SavedStateOverrides;

public:
    CK_PROPERTY(_CurrentStateClass);
    CK_PROPERTY(_Seq);
    CK_PROPERTY(_CurrentStateFingerprint);
    CK_PROPERTY(_RunStatus);
    CK_PROPERTY(_SavedStateOverrides);
};

// --------------------------------------------------------------------------------------------------------------------
