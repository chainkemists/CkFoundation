#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/World/CkEcsWorld.h"

#include <Subsystems/WorldSubsystem.h>
#include <GameFramework/Info.h>

#include "CkEntityReplicationDriver_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKNET_API UCk_EntityReplicationDriver_Subsystem_UE : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityReplicationDriver_Subsystem_UE);

    void Initialize(
        FSubsystemCollectionBase& Collection) override;

    /** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
    auto OnWorldBeginPlay(UWorld& InWorld) -> void override;

    auto Get_NextAvailableReplicationDriver() -> class UCk_Fragment_EntityReplicationDriver_Rep*;
    auto Get_ReplicationDriverWithName(FName InName) -> class UCk_Fragment_EntityReplicationDriver_Rep*;

    auto CreateEntityReplicationDrivers(AActor* InOutermostActor) -> void;

private:
    UPROPERTY(Transient)
    TArray<class UCk_Fragment_EntityReplicationDriver_Rep*> _ReplicationDrivers;
    int32 _NextAvailableReplicationDriver = 0;

    TMap<FName, class UCk_Fragment_EntityReplicationDriver_Rep*> _ReplicationDriversMap;
};

// --------------------------------------------------------------------------------------------------------------------
