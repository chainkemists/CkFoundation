#pragma once

#include "CkStateMachine/Net/CkStateMachine_NetContext.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkStateMachine_RepData.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Reflected save-only mirror of ck::FFragment_Sm_StateOverrides::FEntry — never on the wire; only
// Produce fills it and only HydrationApply consumes it. BlueprintType is load-bearing: the parent
// RepData shapes' generated accessors are AS-registered and reference this type, and a bare USTRUCT
// leaves it unregistered and breaks the whole AS compile at PIE start.
USTRUCT(BlueprintType)
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

// Replication payload for SMs in WithHistory mode: rolling window of transition events (RingSize),
// current run status, initial-state fingerprint. _InitialStateFingerprint is stamped by the publisher
// backfill but never consumed by a receive path — informational only (read by test support).
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

    // Save-only: filled by the save-transport Produce handler, consumed by HydrationApply on load.
    // NEVER filled by the wire publish paths — stays an empty array on every replicated delta.
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

    // Save-only: filled by the save-transport Produce handler, consumed by HydrationApply on load.
    // NEVER filled by the wire publish paths — stays an empty array on every replicated delta.
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
