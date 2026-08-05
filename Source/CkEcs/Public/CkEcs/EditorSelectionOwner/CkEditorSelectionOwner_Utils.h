#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Fragment.h"
#include "CkEcs/Handle/CkHandle.h"

#include <CoreMinimal.h>

#include "CkEditorSelectionOwner_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;
class UWorld;

#if WITH_EDITOR
DECLARE_MULTICAST_DELEGATE_OneParam(FCk_EditorSelectionOwner_OnRefreshRequested, const AActor*);
#endif

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_EditorSelectionOwner_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EditorSelectionOwner_UE);

#if WITH_EDITOR
public:
    static auto
    Has(
        const FCk_Handle& InHandle) -> bool;

    // Live owner actor; nullptr when the entity is not part of an editor preview or the owner
    // actor is gone.
    static auto
    TryGet_SelectionOwner(
        const FCk_Handle& InHandle) -> AActor*;

    // Owner identity for per-owner cache keys (compares/hashes by object index + serial, so lookups
    // survive the owner's destruction). Default when the entity is not part of an editor preview.
    static auto
    TryGet_SelectionOwnerWeak(
        const FCk_Handle& InHandle) -> TWeakObjectPtr<AActor>;

    // Re-setup with the same owner is a no-op, a different owner ensures. Called by
    // Request_SetupEntityWithLifetimeOwner so every lifetime-descendant inherits the fragment at creation,
    // and by preview owners to stamp the root BEFORE any of its descendants are created.
    static auto
    Request_SetupEntityWithEditorSelectionOwner(
        FCk_Handle& InNewEntity,
        const TWeakObjectPtr<AActor>& InOwnerActor) -> void;

    // Editor-world helper actors that host preview visuals for an owner register here: they are NOT
    // attached to the owner, so the engine's attached-actor selection propagation cannot reach them.
    static auto
    RegisterProxyActor(
        AActor* InOwnerActor,
        AActor* InProxyActor) -> void;

    static auto
    PushOwnerSelectionToProxies(
        const AActor* InOwnerActor) -> void;

    // Backend-neutral editor notification. Upper-tier retained visualizers subscribe here so
    // selection and rebuild changes can reconcile once, without making CkEcs depend on a renderer.
    static auto
    Get_OnRefreshRequested() -> FCk_EditorSelectionOwner_OnRefreshRequested&;

    // Resolves the entity's selection owner and returns its per-owner host actor. nullptr outside editor
    // worlds or when no owner is stamped — callers then fall back to their non-preview hosting.
    static auto
    TryGet_SelectionProxyHostActor(
        const UWorld* InWorld,
        const FCk_Handle& InHandle) -> AActor*;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
