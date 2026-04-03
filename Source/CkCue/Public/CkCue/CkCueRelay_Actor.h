#pragma once

#include "CkActorRelay/CkActorRelay_Actor.h"

#include "CkEcs/Handle/CkHandle.h"

#include <StructUtils/InstancedStruct.h>

#include "CkCueRelay_Actor.generated.h"

class UCk_CueExecutor_Subsystem_Base_UE;

/*─────────────────────────────────────────────────────────────────────────────┐
│                              CUE RELAY ACTOR                                 │
└─────────────────────────────────────────────────────────────────────────────*/

UCLASS()
class CKCUE_API ACk_CueRelay_UE : public ACk_ActorRelay_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_CueRelay_UE);

public:
    UFUNCTION(Server, Unreliable)
    void Server_RequestExecuteCue(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams);

    UFUNCTION(Server, Reliable)
    void Server_RequestExecuteCue_Reliable(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams);

    UFUNCTION(Server, Unreliable)
    void Server_RequestExecuteCue_ServerOnly(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams);

    UFUNCTION(Server, Reliable)
    void Server_RequestExecuteCue_ServerOnly_Reliable(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams);

    UFUNCTION(Server, Unreliable)
    void Server_RequestExecuteCue_ExcludingSender(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams);

    UFUNCTION(Server, Reliable)
    void Server_RequestExecuteCue_ExcludingSender_Reliable(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_ExecuteCue(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_ExecuteCue_Reliable(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_ExecuteCue_ExcludingSender(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams,
        APlayerState* InExcludedPlayerState);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_ExecuteCue_ExcludingSender_Reliable(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams,
        APlayerState* InExcludedPlayerState);

private:
    auto
    DoGetCueExecutorSubsystem() const -> UCk_CueExecutor_Subsystem_Base_UE*;
};

// --------------------------------------------------------------------------------------------------------------------
