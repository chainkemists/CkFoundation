#pragma once

#include "CkActorRelay/CkActorRelay_GroupSubsystem.h"

#include "CkVoiceChatControlRelay_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// The base class auto-spawns one channel per player at PostLogin (plus one for the server) and
// tears it down on logout.
UCLASS()
class CKVOICECHAT_API UCk_VoiceChatControlRelay_Subsystem_UE : public UCk_ActorRelay_Group_Subsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_VoiceChatControlRelay_Subsystem_UE);

public:
    auto
    Get_GroupTag() const -> FGameplayTag override;

    auto
    Get_ActorClass() const -> TSubclassOf<ACk_ActorRelay_UE> override;
};

// --------------------------------------------------------------------------------------------------------------------
