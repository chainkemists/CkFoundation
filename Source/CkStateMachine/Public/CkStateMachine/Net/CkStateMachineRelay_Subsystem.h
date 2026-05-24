#pragma once

#include "CkActorRelay/CkActorRelay_GroupSubsystem.h"

#include "CkStateMachineRelay_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Group subsystem that owns the per-player ACk_StateMachineRelay_UE channels used by
// OwningClientAuthoritative state machines to push client→server RPCs.
//
// The base class (UCk_ActorRelay_Group_Subsystem_Base_UE) auto-spawns one channel per
// player at PostLogin (and one for the server itself). Channels live for the player's
// session and are torn down on logout.
UCLASS()
class CKSTATEMACHINE_API UCk_StateMachineRelay_Subsystem_UE : public UCk_ActorRelay_Group_Subsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_StateMachineRelay_Subsystem_UE);

public:
    auto
    Get_GroupTag() const -> FGameplayTag override;

    auto
    Get_ActorClass() const -> TSubclassOf<ACk_ActorRelay_UE> override;
};

// --------------------------------------------------------------------------------------------------------------------
