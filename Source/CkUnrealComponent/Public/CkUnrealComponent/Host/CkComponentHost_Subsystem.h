#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"

#include "CkComponentHost_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Editor-world host for scene components that belong to a placed actor's preview entity (see
// ck::FFragment_EditorSelectionOwner). One is spawned per selection owner so a viewport click on a
// hosted primitive redirects selection to that owner via the engine's selection-parent mechanism —
// clicking the preview mesh then behaves exactly like clicking the owner itself.
UCLASS(NotBlueprintable, NotPlaceable)
class CKUNREALCOMPONENT_API ACk_ComponentHost_Actor_UE : public AActor
{
    GENERATED_BODY()

public:
    friend class UCk_ComponentHost_Subsystem_UE;

#if WITH_EDITOR
public:
    auto
    IsSelectionChild() const -> bool override;

    auto
    GetSelectionParent() const -> AActor* override;
#endif

#if WITH_EDITORONLY_DATA
private:
    // Non-UPROPERTY on purpose: this actor is transient and must never round-trip through the
    // transaction buffer; the owner is a level actor whose lifetime the weak ptr observes.
    TWeakObjectPtr<AActor> _EditorSelectionOwner;
#endif
};

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

#if WITH_EDITOR
    // Editor-world variant: components of a placed actor's preview entity are hosted on a
    // per-owner actor instead of the shared host, so viewport clicks on them can redirect
    // selection to the owner (a shared host would make every preview mesh select the same
    // meaningless actor). Falls back to the shared host when InSelectionOwner is invalid.
    auto
    Get_HostActor_ForEditorSelectionOwner(AActor* InSelectionOwner) -> AActor*;
#endif

private:
    UPROPERTY(Transient)
    TObjectPtr<AActor> _HostActor = nullptr;

#if WITH_EDITORONLY_DATA
private:
    // Weak values: the actors are owned by the world; entries self-heal via validity checks.
    TMap<FObjectKey, TWeakObjectPtr<ACk_ComponentHost_Actor_UE>> _PerOwnerHostActors;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
