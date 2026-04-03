#pragma once

#include "CkActorRelay_Fragment_Data.h"
#include "CkActorRelay_Actor.h"

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include <GameFramework/GameModeBase.h>

#include "CkActorRelay_GroupSubsystem.generated.h"

/*-----------------------------------------------------------------------------
                     ACTOR RELAY GROUP SUBSYSTEM BASE
------------------------------------------------------------------------------*/

UCLASS(Abstract)
class CKACTORRELAY_API UCk_ActorRelay_Group_Subsystem_Base_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ActorRelay_Group_Subsystem_Base_UE);

    friend class ACk_ActorRelay_UE;

public:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;

    /*-------------------------------------------------------------------------
                          VIRTUAL CONFIG METHODS
    --------------------------------------------------------------------------*/

public:
    virtual auto Get_GroupTag() const -> FGameplayTag
    CK_PURE_VIRTUAL(UCk_ActorRelay_Group_Subsystem_Base_UE::Get_GroupTag, return {});

    virtual auto Get_ActorClass() const -> TSubclassOf<ACk_ActorRelay_UE>
    CK_PURE_VIRTUAL(UCk_ActorRelay_Group_Subsystem_Base_UE::Get_ActorClass, return {});

    virtual auto
    Get_OwnershipPolicy() const -> ECk_ActorRelay_OwnershipPolicy;

    virtual auto
    Get_ChannelCount() const -> int32;

    virtual auto
    Get_MaxEntitiesPerChannel() const -> int32;

    virtual auto
    Get_SelectionAlgorithm() const -> ECk_ActorRelay_SelectionAlgorithm;

    virtual auto
    Get_DisconnectPolicy() const -> ECk_ActorRelay_DisconnectPolicy;

    /*-------------------------------------------------------------------------
                              CONSUMER API
    --------------------------------------------------------------------------*/

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|ActorRelay",
              DisplayName = "[Ck][ActorRelay] Acquire Channel")
    FCk_ActorRelay_ChannelResult
    Request_AcquireChannel();

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|ActorRelay",
              DisplayName = "[Ck][ActorRelay] Acquire Channel For Player")
    FCk_ActorRelay_ChannelResult
    Request_AcquireChannel_ForPlayer(
        APlayerState* InPlayerState);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|ActorRelay",
              DisplayName = "[Ck][ActorRelay] Acquire Any Channel")
    FCk_ActorRelay_ChannelResult
    Request_AcquireAnyChannel();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|ActorRelay",
              DisplayName = "[Ck][ActorRelay] Get Channel Count Active")
    int32
    Get_ChannelCount_Active() const;

    /*-------------------------------------------------------------------------
                              INTERNALS
    --------------------------------------------------------------------------*/

private:
    auto DoRegisterChannelActor(ACk_ActorRelay_UE* InChannelActor) -> void;

    auto DoSpawnChannels_Server() -> void;
    auto DoSpawnChannels_ForPlayer(APlayerController* InPlayerController) -> void;
    auto DoDestroyChannels_ForPlayer(APlayerState* InPlayerState) -> void;

    auto DoSelectChannel_FromPool(
        TArray<TObjectPtr<ACk_ActorRelay_UE>>& InPool,
        int32& InOutRoundRobinIndex) -> FCk_ActorRelay_ChannelResult;

    auto DoGetEntityCountOnChannel(
        ACk_ActorRelay_UE* InChannelActor) const -> int32;

private:
    auto OnPostLoadMapWithWorld(UWorld* InWorld) -> void;
    auto OnPostLoginEvent(AGameModeBase* InGameMode, APlayerController* InNewPlayer) -> void;
    auto OnPlayerLogout(AGameModeBase* InGameMode, AController* InController) -> void;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<ACk_ActorRelay_UE>> _ServerChannels;
    int32 _ServerRoundRobinIndex = 0;

    TMap<TWeakObjectPtr<APlayerState>, TArray<TObjectPtr<ACk_ActorRelay_UE>>> _PlayerChannels;
    TMap<TWeakObjectPtr<APlayerState>, int32> _PlayerRoundRobinIndices;

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<APlayerController>> _RegisteredPlayerControllers;

private:
    FDelegateHandle _PostLoadMapWithWorldDelegateHandle;
    FDelegateHandle _PostLoginEventDelegateHandle;
    FDelegateHandle _LogoutEventDelegateHandle;
};

// --------------------------------------------------------------------------------------------------------------------
