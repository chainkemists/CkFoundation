#pragma once

#include "CkStateMachine/Net/CkStateMachine_NetContext.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkStateMachine_RepData.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Replication payload for SMs in WithHistory mode. Carries a rolling window of transition events
// (size 64), the current run status, and the initial-state fingerprint for the spec §9.5
// Setup-time check.
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

public:
    CK_PROPERTY(_History);
    CK_PROPERTY(_RunStatus);
    CK_PROPERTY(_InitialStateFingerprint);
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

public:
    CK_PROPERTY(_CurrentStateClass);
    CK_PROPERTY(_Seq);
    CK_PROPERTY(_CurrentStateFingerprint);
    CK_PROPERTY(_RunStatus);
};

// --------------------------------------------------------------------------------------------------------------------
