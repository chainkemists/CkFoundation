#pragma once

#include "CkActorRelay/CkActorRelay_Actor.h"

#include "CkStateMachine/Net/CkStateMachine_NetContext.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkStateMachineRelay_Actor.generated.h"

class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

// Per-actor RPC relay for StateMachine replication.
//
// Carries client→server RPCs only — server→client traffic uses replicated fragments via
// FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy (wired in Phase 5).
//
// Acquired per owning actor by SMs that opt into OwningClientAuthoritative authority via
// FCk_Fragment_StateMachine_ParamsData::_AuthorityModel. Phase 4 only provides the actor +
// RPC stubs; Phase 10 wires the actor into the owning-client authoritative path.
UCLASS()
class CKSTATEMACHINE_API ACk_StateMachineRelay_UE : public ACk_ActorRelay_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_StateMachineRelay_UE);

public:
    // Owning client pushes a batched transition history for one SM.
    // The server applies the batch in order if the seqs are contiguous past its watermark,
    // otherwise rejects and snaps the client back (wired in Phase 10).
    UFUNCTION(Server, Reliable)
    void Server_PushTransitionBatch(
        FCk_Handle InSMHandle,
        const TArray<FCk_Sm_TransitionEvent>& InBatch);

    // Owning client pushes the current state (WithoutHistory replication model).
    // Seq and fingerprint are int32 because UHT rejects uint32 for Blueprint-visible UPROPERTY,
    // and we keep RPC param types consistent with the FCk_Sm_TransitionEvent storage shape so
    // the same values flow across the two transports without conversion.
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
