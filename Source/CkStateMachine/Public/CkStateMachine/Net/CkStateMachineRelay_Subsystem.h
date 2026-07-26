#pragma once

#include "CkActorRelay/CkActorRelay_GroupSubsystem.h"

#include "CkStateMachineRelay_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Owns the per-player ACk_StateMachineRelay_UE channels that OwningClientAuthoritative state
// machines push client→server RPCs through. The base class auto-spawns one channel per player at
// PostLogin (plus one for the server itself); channels are torn down on logout.
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
