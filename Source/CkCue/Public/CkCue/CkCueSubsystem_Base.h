#pragma once

#include "CkCue_EntityScript.h"

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"
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
    ServerOnly
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
│                              CUE EXECUTOR ACTOR                              │
└─────────────────────────────────────────────────────────────────────────────*/

UCLASS()
class CKCUE_API ACk_CueExecutor_UE : public AActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_CueExecutor_UE);

    friend class UCk_CueExecutor_Subsystem_Base_UE;

public:
    ACk_CueExecutor_UE();

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
    void Request_ExecuteCue(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams);

    UFUNCTION(NetMulticast, Reliable)
    void Request_ExecuteCue_Reliable(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams);

    UFUNCTION(NetMulticast, Unreliable)
    void Request_ExecuteCue_ExcludingSender(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams,
        APlayerController* InExcludedPC);

    UFUNCTION(NetMulticast, Reliable)
    void Request_ExecuteCue_ExcludingSender_Reliable(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams,
        APlayerController* InExcludedPC);

protected:
    auto BeginPlay() -> void override;

    auto GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

private:
    auto InjectCueExecutorSubsystemClass(
        TSubclassOf<class UCk_CueExecutor_Subsystem_Base_UE> InCueExecutorSubsystemClass) -> void;

private:
    UFUNCTION()
    void OnRep_CueExecutorSubsystemClass();

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess))
    TObjectPtr<class UCk_EntityBridge_ActorComponent_UE> _EntityBridge;

    UPROPERTY(ReplicatedUsing = OnRep_CueExecutorSubsystemClass)
    TSubclassOf<class UCk_CueExecutor_Subsystem_Base_UE> _Subsystem_CueExecutorClass;

    UPROPERTY(Transient)
    TWeakObjectPtr<class UCk_CueExecutor_Subsystem_Base_UE> _Subsystem_CueExecutor;

    UPROPERTY(Transient)
    TWeakObjectPtr<class UCk_EcsWorld_Subsystem_UE> _Subsystem_EcsWorld;
};

/*─────────────────────────────────────────────────────────────────────────────┐
│                         CUE EXECUTOR SUBSYSTEM BASE                          │
└─────────────────────────────────────────────────────────────────────────────*/

UCLASS(Abstract)
class CKCUE_API UCk_CueExecutor_Subsystem_Base_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

    friend class ACk_CueExecutor_UE;

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
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::MulticastToClients);

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
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::MulticastToClients);

    UFUNCTION(BlueprintCallable)
    FCk_Handle_PendingEntityScript Request_ExecuteCue_Local(
        const FCk_Handle& InOwnerEntity,
        UPARAM(meta = (Categories = "Cue")) FGameplayTag InCueName,
        FInstancedStruct InSpawnParams);

public:
    virtual auto Get_CueSubsystemClass() const -> TSubclassOf<class UCk_CueSubsystem_Base_UE>
    CK_PURE_VIRTUAL(UCk_CueExecutor_Subsystem_Base_UE::Get_CueSubsystemClass, return {});

    virtual auto Get_DedicatedServerPolicy() const -> ECk_Cue_DedicatedServerPolicy
    CK_PURE_VIRTUAL(UCk_CueExecutor_Subsystem_Base_UE::Get_DedicatedServerPolicy, return ECk_Cue_DedicatedServerPolicy::CosmeticOnly);

private:
    auto DoSpawnCueExecutorActorsForPlayerController(APlayerController* InPlayerController) -> void;
    auto OnPostLoadMapWithWorld(UWorld* InWorld) -> void;
    auto OnPostLoginEvent(AGameModeBase* GameMode, APlayerController* NewPlayer) -> void;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<ACk_CueExecutor_UE>> _CueExecutors;
    int32 _NextAvailableExecutor = 0;

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<APlayerController>> _ValidPlayerControllers;

    TMap<TWeakObjectPtr<APlayerController>, TWeakObjectPtr<ACk_CueExecutor_UE>> _ExecutorsByPlayerController;

private:
    FDelegateHandle _PostLoadMapWithWorldDelegateHandle;
    FDelegateHandle _PostLoginEventDelegateHandle;
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
