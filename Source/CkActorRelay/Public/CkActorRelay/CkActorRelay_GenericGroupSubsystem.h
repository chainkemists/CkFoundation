#pragma once

#include "CkActorRelay_GroupSubsystem.h"

#include "CkActorRelay_GenericGroupSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_GenericActorRelay")
class CKACTORRELAY_API UCk_ActorRelay_GenericGroup_Subsystem_UE : public UCk_ActorRelay_Group_Subsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ActorRelay_GenericGroup_Subsystem_UE);

public:
    auto
    Get_GroupTag() const -> FGameplayTag override;

    auto
    Get_ActorClass() const -> TSubclassOf<ACk_ActorRelay_UE> override;

    auto
    Get_OwnershipPolicy() const -> ECk_ActorRelay_OwnershipPolicy override;

    auto
    Get_ChannelCount() const -> int32 override;

    auto
    Get_SelectionAlgorithm() const -> ECk_ActorRelay_SelectionAlgorithm override;
};

// --------------------------------------------------------------------------------------------------------------------
