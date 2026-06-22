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

    // Lazily-spawned per-world Actor that OWNS the scene components created by the
    // UnrealComponent feature. A component can only register with the navigation
    // octree when GetOwner() is non-null (UNavigationSystemV1::RegisterComponentToNavOctree
    // early-outs otherwise), so world-hosted/owner-less components could never cut the
    // navmesh. Owning them under this Actor restores nav relevance; the engine still
    // gates actual geometry export on collision (query + Pawn block), so cosmetic
    // NoCollision meshes remain harmless.
    auto
    Get_HostActor() -> AActor*;

private:
    UPROPERTY(Transient)
    TObjectPtr<AActor> _HostActor = nullptr;
};

// --------------------------------------------------------------------------------------------------------------------
