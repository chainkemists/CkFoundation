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
    //
    // Editor-preview components do NOT host here — they go to the per-selection-owner host
    // (ck::editor_selection_owner::TryGet_SelectionProxyHostActor) so viewport clicks on them
    // redirect selection to the placed actor that owns the preview.
    auto
    Get_HostActor() -> AActor*;

private:
    UPROPERTY(Transient)
    TObjectPtr<AActor> _HostActor = nullptr;
};

// --------------------------------------------------------------------------------------------------------------------
