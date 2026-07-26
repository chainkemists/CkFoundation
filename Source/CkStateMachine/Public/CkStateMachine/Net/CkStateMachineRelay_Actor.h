#pragma once

#include "CkActorRelay/CkActorRelay_Actor.h"

#include "CkStateMachine/Net/CkStateMachine_NetContext.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkStateMachineRelay_Actor.generated.h"

class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

// Per-actor RPC relay for StateMachine replication. Client→server only — server→client traffic
// uses replicated fragments. Acquired per owning actor by SMs whose _AuthorityModel opts into
// OwningClientAuthoritative; every push is authorized server-side by the .cpp's
// DoGet_IsAuthorizedOwningClientPush before entering the standard replay/commit pipeline.
UCLASS()
class CKSTATEMACHINE_API ACk_StateMachineRelay_UE : public ACk_ActorRelay_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_StateMachineRelay_UE);

public:
    // Owning client pushes a batched transition history. Seq dedup happens at drain (via
    // FFragment_Sm_ClientReplayState); there is no contiguity check or client snap-back.
    UFUNCTION(Server, Reliable)
    void Server_PushTransitionBatch(
        FCk_Handle InSMHandle,
        const TArray<FCk_Sm_TransitionEvent>& InBatch);

    // Owning client pushes the current state (WithoutHistory replication model). Seq and fingerprint
    // are int32 because UHT rejects uint32 for Blueprint-visible UPROPERTY, matching
    // FCk_Sm_TransitionEvent's storage so the values cross both transports without conversion.
    UFUNCTION(Server, Reliable)
    void Server_PushCurrentState(
        FCk_Handle InSMHandle,
        TSubclassOf<UCk_SmState_EntityScript> InCurrentStateClass,
        int32 InSeq,
        int32 InCurrentStateFingerprint);

    // Owning client pushes a run-status change (Start / Stop intent).
    UFUNCTION(Server, Reliable)
    void Server_PushRunStatus(
        FCk_Handle InSMHandle,
        ECk_SmRunStatus InRunStatus);
};

// --------------------------------------------------------------------------------------------------------------------
