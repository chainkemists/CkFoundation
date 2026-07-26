#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "CkComponentHost_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUNREALCOMPONENT_API UCk_ComponentHost_Subsystem_UE : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    static auto
    Get(UWorld* InWorld) -> UCk_ComponentHost_Subsystem_UE*;

    // Lazily-spawned per-world Actor that OWNS the scene components created by the UnrealComponent
    // feature: UNavigationSystemV1::RegisterComponentToNavOctree early-outs when GetOwner() is null,
    // so owner-less components could never cut the navmesh. Editor previews host elsewhere — on the
    // per-selection-owner proxy (UCk_Utils_EditorSelectionOwner_UE::TryGet_SelectionProxyHostActor).
    auto
    Get_HostActor() -> AActor*;

private:
    UPROPERTY(Transient)
    TObjectPtr<AActor> _HostActor = nullptr;
};

// --------------------------------------------------------------------------------------------------------------------
