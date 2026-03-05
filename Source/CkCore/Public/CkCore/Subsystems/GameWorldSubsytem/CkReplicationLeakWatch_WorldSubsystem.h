#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include <CoreMinimal.h>
#include <UObject/ObjectKey.h>

#include "CkReplicationLeakWatch_WorldSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKCORE_API UCk_ReplicationLeakWatch_WorldSubsystem_UE : public UCk_Game_TickableWorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ReplicationLeakWatch_WorldSubsystem_UE);

public:
    auto Tick(float InDeltaSeconds) -> void override;
    auto GetStatId() const -> TStatId override;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Networking|ReplicationWatch")
    void Request_StartMonitoring();

    UFUNCTION(BlueprintCallable, Category = "Ck|Networking|ReplicationWatch")
    void Request_StopMonitoring();

    UFUNCTION(BlueprintCallable, Category = "Ck|Networking|ReplicationWatch")
    void Request_SampleNow();

    UFUNCTION(BlueprintCallable, Category = "Ck|Networking|ReplicationWatch")
    void Request_DumpReport();

    UFUNCTION(BlueprintCallable, Category = "Ck|Networking|ReplicationWatch")
    void Request_ResetBaseline();

private:
    enum class EObjectType : uint8
    {
        Actor,
        Component,
        UObject
    };

    struct FTrackedReplicatedObject
    {
        TWeakObjectPtr<UObject> Object;
        TWeakObjectPtr<UObject> Owner;
        EObjectType Type = EObjectType::UObject;
        double FirstSeenAtSeconds = 0.0;
        double LastSeenAtSeconds = 0.0;
        int32 SeenSamples = 0;
        bool bOwnerReplicated = false;
    };

    struct FReplicationSnapshot
    {
        double TimestampSeconds = 0.0;
        int32 ReplicatedActorCount = 0;
        int32 ReplicatedComponentCount = 0;
        int32 ReplicatedObjectCount = 0;
        int32 TotalTrackedCount = 0;

        TMap<UClass*, int32> ActorClassCounts;
        TMap<UClass*, int32> ComponentClassCounts;
        TMap<UClass*, int32> ObjectClassCounts;
        TMap<UObject*, int32> OwnerCounts;
    };

private:
    auto IsMonitoringEnabledByCVar() const -> bool;
    auto GetCurrentSampleIntervalSeconds() const -> float;
    auto GetCurrentLeakAgeThresholdSeconds() const -> float;
    auto GetCurrentGrowthThresholdCount() const -> int32;
    auto GetCurrentTopN() const -> int32;
    auto GetShouldLogEachSuspect() const -> bool;
    auto GetShouldAutoDumpOnGrowth() const -> bool;

    auto IsObjectInCurrentWorld(UObject* InObject) const -> bool;
    auto ResolveReplicatedOwner(UObject* InObject) const -> UObject*;
    auto IsReplicatedOwner(UObject* InObject) const -> bool;
    auto GetObjectTypeLabel(EObjectType InType) const -> const TCHAR*;

    auto GatherReplicatedActors(TArray<AActor*>& OutActors) const -> void;
    auto GatherReplicatedComponents(const TArray<AActor*>& InActors, TArray<UActorComponent*>& OutComponents) const -> void;
    auto GatherReplicatedUObjects(TArray<UObject*>& OutObjects) const -> void;

    auto TouchTracked(UObject* InObject, UObject* InOwner, EObjectType InType, double InNowSeconds) -> void;
    auto CompactTrackedMap() -> void;

    auto BuildSnapshot(double InNowSeconds) -> FReplicationSnapshot;
    auto DumpSnapshot(const FReplicationSnapshot& InSnapshot) const -> void;
    auto DumpSuspects(double InNowSeconds) const -> void;

private:
    bool _IsMonitoring = false;
    double _TimeSinceLastSampleSeconds = 0.0;
    int32 _NumSamples = 0;
    int32 _LastTotalTrackedCount = 0;

    FReplicationSnapshot _LastSnapshot;
    TMap<FObjectKey, FTrackedReplicatedObject> _TrackedObjects;
};

// --------------------------------------------------------------------------------------------------------------------
