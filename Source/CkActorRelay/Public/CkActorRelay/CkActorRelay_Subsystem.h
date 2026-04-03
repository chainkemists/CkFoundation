#pragma once

#include "CkActorRelay_Fragment_Data.h"

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkActorRelay_Subsystem.generated.h"

class UCk_ActorRelay_Group_Subsystem_Base_UE;

/*-----------------------------------------------------------------------------
                       ACTOR RELAY SUBSYSTEM (REGISTRY)
------------------------------------------------------------------------------*/

UCLASS()
class CKACTORRELAY_API UCk_ActorRelay_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ActorRelay_Subsystem_UE);

    friend class UCk_ActorRelay_Group_Subsystem_Base_UE;

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|ActorRelay",
              DisplayName = "[Ck][ActorRelay] Get Group Subsystem")
    UCk_ActorRelay_Group_Subsystem_Base_UE*
    Get_GroupSubsystem(
        UPARAM(meta = (Categories = "ActorRelay")) const FGameplayTag& InGroupTag) const;

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|ActorRelay",
              DisplayName = "[Ck][ActorRelay] Acquire Channel")
    FCk_ActorRelay_ChannelResult
    Request_AcquireChannel(
        UPARAM(meta = (Categories = "ActorRelay")) const FGameplayTag& InGroupTag);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|ActorRelay",
              DisplayName = "[Ck][ActorRelay] Acquire Channel For Player")
    FCk_ActorRelay_ChannelResult
    Request_AcquireChannel_ForPlayer(
        UPARAM(meta = (Categories = "ActorRelay")) const FGameplayTag& InGroupTag,
        APlayerState* InPlayerState);

private:
    auto
    DoRegisterGroup(
        UCk_ActorRelay_Group_Subsystem_Base_UE* InGroupSubsystem) -> void;

    auto
    DoUnregisterGroup(
        UCk_ActorRelay_Group_Subsystem_Base_UE* InGroupSubsystem) -> void;

private:
    TMap<FGameplayTag, TWeakObjectPtr<UCk_ActorRelay_Group_Subsystem_Base_UE>> _RegisteredGroups;
};

// --------------------------------------------------------------------------------------------------------------------
