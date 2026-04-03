#pragma once

#include "CkCue_EntityScript.h"
#include "CkCueRelay_Actor.h"

#include "CkActorRelay/CkActorRelay_GroupSubsystem.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"

#include <Subsystems/EngineSubsystem.h>
#include <GameFramework/GameModeBase.h>

#include "CkCueSubsystem_Base.generated.h"

/*─────────────────────────────────────────────────────────────────────────────┐
│                            RELIABILITY POLICY                                │
└─────────────────────────────────────────────────────────────────────────────*/

UENUM(BlueprintType)
enum class ECk_Cue_ReliabilityPolicy : uint8
{
    Unreliable,
    Reliable
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Cue_ReliabilityPolicy);

/*─────────────────────────────────────────────────────────────────────────────┐
│                           MULTICAST POLICY                                   │
└─────────────────────────────────────────────────────────────────────────────*/

UENUM(BlueprintType)
enum class ECk_Cue_MulticastPolicy : uint8
{
    MulticastToClients,
    MulticastToOtherClients,
    ServerOnly,
    ServerAndSelf
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Cue_MulticastPolicy);

/*─────────────────────────────────────────────────────────────────────────────┐
│                         DEDICATED SERVER POLICY                              │
└─────────────────────────────────────────────────────────────────────────────*/

UENUM(BlueprintType)
enum class ECk_Cue_DedicatedServerPolicy : uint8
{
    CosmeticOnly,
    GameplayRelevant
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Cue_DedicatedServerPolicy);

/*─────────────────────────────────────────────────────────────────────────────┐
│                         CUE EXECUTOR SUBSYSTEM BASE                          │
└─────────────────────────────────────────────────────────────────────────────*/

UCLASS(Abstract)
class CKCUE_API UCk_CueExecutor_Subsystem_Base_UE : public UCk_ActorRelay_Group_Subsystem_Base_UE
{
    GENERATED_BODY()

    friend class ACk_CueRelay_UE;

public:
    CK_GENERATED_BODY(UCk_CueExecutor_Subsystem_Base_UE);

public:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;

    UFUNCTION(BlueprintCallable)
    FCk_Handle_PendingEntityScript Request_ExecuteCue_Transient(
        UPARAM(meta = (Categories = "Cue")) FGameplayTag InCueName,
        FInstancedStruct InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliability = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::MulticastToClients,
        ECk_Cue_ExecutionPolicy InExecutionPolicy = ECk_Cue_ExecutionPolicy::Replicated);

    UFUNCTION(BlueprintCallable)
    FCk_Handle_PendingEntityScript Request_ExecuteCue_Transient_Local(
        UPARAM(meta = (Categories = "Cue")) FGameplayTag InCueName,
        FInstancedStruct InSpawnParams);

    UFUNCTION(BlueprintCallable)
    FCk_Handle_PendingEntityScript Request_ExecuteCue(
        const FCk_Handle& InOwnerEntity,
        UPARAM(meta = (Categories = "Cue")) FGameplayTag InCueName,
        FInstancedStruct InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliability = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::MulticastToClients,
        ECk_Cue_ExecutionPolicy InExecutionPolicy = ECk_Cue_ExecutionPolicy::Replicated);

    UFUNCTION(BlueprintCallable)
    FCk_Handle_PendingEntityScript Request_ExecuteCue_Local(
        const FCk_Handle& InOwnerEntity,
        UPARAM(meta = (Categories = "Cue")) FGameplayTag InCueName,
        FInstancedStruct InSpawnParams);

    /*-------------------------------------------------------------------------
                      ACTOR RELAY CONFIG OVERRIDES
    --------------------------------------------------------------------------*/

public:
    auto Get_ActorClass() const -> TSubclassOf<ACk_ActorRelay_UE> override;

    /*-------------------------------------------------------------------------
                      CUE-SPECIFIC PURE VIRTUALS
    --------------------------------------------------------------------------*/

public:
    virtual auto Get_CueSubsystemClass() const -> TSubclassOf<class UCk_CueSubsystem_Base_UE>
    CK_PURE_VIRTUAL(UCk_CueExecutor_Subsystem_Base_UE::Get_CueSubsystemClass, return {});

    virtual auto Get_DedicatedServerPolicy() const -> ECk_Cue_DedicatedServerPolicy
    CK_PURE_VIRTUAL(UCk_CueExecutor_Subsystem_Base_UE::Get_DedicatedServerPolicy, return ECk_Cue_DedicatedServerPolicy::CosmeticOnly);

private:
    auto DoProcessPendingCues() -> void;
    auto DoCheckPendingCueTimeout(float InDeltaTime) -> bool;
    auto DoAcquireCueRelay_ForClient() -> ACk_CueRelay_UE*;

private:
    struct FCk_PendingCueRequest
    {
        CK_GENERATED_BODY(FCk_PendingCueRequest);

        FCk_Handle OwnerEntity;
        FGameplayTag CueName;
        FInstancedStruct SpawnParams;
        ECk_Cue_ReliabilityPolicy Reliability;
        ECk_Cue_MulticastPolicy MulticastPolicy;
        ECk_Cue_ExecutionPolicy ExecutionPolicy;
        double TimeRequested = FPlatformTime::Seconds();

    public:
        CK_DEFINE_CONSTRUCTOR(FCk_PendingCueRequest, OwnerEntity, CueName, SpawnParams, Reliability, MulticastPolicy, ExecutionPolicy);
    };

private:
    TArray<FCk_PendingCueRequest> _PendingCues;
    FTSTicker::FDelegateHandle _PendingCueTimeoutTickerHandle;
};

/*─────────────────────────────────────────────────────────────────────────────┐
│                            CUE SUBSYSTEM BASE                                │
└─────────────────────────────────────────────────────────────────────────────*/

UCLASS(Abstract)
class CKCUE_API UCk_CueSubsystem_Base_UE : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CueSubsystem_Base_UE);

public:
    auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    auto Deinitialize() -> void override;

public:
    auto Request_PopulateAllCues() -> void;

public:
    virtual auto Get_CueBaseClass() const -> TSubclassOf<UCk_CueBase_EntityScript>
    CK_PURE_VIRTUAL(UCk_CueSubsystem_Base_UE::Get_CueBaseClass, return {});

private:
    auto DoOnEngineInitComplete() -> void;
    auto DoHandleAssetAddedDeleted(const FAssetData&) -> void;
    auto Request_ProcessAssetUpdate(const FAssetData& InAssetData) -> void;
    auto Request_PopulateBlueprintCues() -> void;
    auto DoHandleRenamed(const FAssetData&, const FString&) -> void;
    auto DoAssetUpdated(const FAssetData&) -> void;

public:
    auto Get_CueEntityScript(const FGameplayTag& InCueName) -> TSubclassOf<UCk_CueBase_EntityScript>;
    auto Get_DiscoveredCues() const -> const TMap<FGameplayTag, TSubclassOf<UCk_CueBase_EntityScript>>&;

private:
    auto DoExecutePopulateAllCues() -> void;
    auto Request_DeferredPopulateAllCues() -> void;
    auto DoTickDeferredDiscovery(float InDeltaTime) -> bool;

private:
    FTSTicker::FDelegateHandle _DiscoveryDeferralTickerHandle;
    int32 _DiscoveryDeferralFramesRemaining = 0;
    static constexpr int32 DISCOVERY_DEFERRAL_FRAMES = 60;

protected:
    UPROPERTY(Transient)
    TMap<FGameplayTag, TSubclassOf<UCk_CueBase_EntityScript>> _DiscoveredCues;
};

/*─────────────────────────────────────────────────────────────────────────────┐
│                         GENERIC CUE IMPLEMENTATIONS                          │
└─────────────────────────────────────────────────────────────────────────────*/

UCLASS(DisplayName = "CkSubsystem_GenericCueExecutor", NotBlueprintable, BlueprintType)
class CKCUE_API UCk_GenericCueExecutor_Subsystem_UE : public UCk_CueExecutor_Subsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GenericCueExecutor_Subsystem_UE);

protected:
    auto Get_GroupTag() const -> FGameplayTag override;
    auto Get_CueSubsystemClass() const -> TSubclassOf<UCk_CueSubsystem_Base_UE> override;
    auto Get_DedicatedServerPolicy() const -> ECk_Cue_DedicatedServerPolicy override;
};

UCLASS(DisplayName = "CkSubsystem_GenericCue", NotBlueprintable, NotBlueprintType)
class CKCUE_API UCk_GenericCueSubsystem_UE : public UCk_CueSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GenericCueSubsystem_UE);

protected:
    auto Get_CueBaseClass() const -> TSubclassOf<UCk_CueBase_EntityScript> override;
};
