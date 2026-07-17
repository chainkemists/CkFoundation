#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Fragment.h"
#include "CkEcs/Handle/CkHandle.h"

#include <CoreMinimal.h>

#include "CkEditorSelectionOwner_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;
class UWorld;

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

    // Owner identity for per-owner cache keys: TWeakObjectPtr compares/hashes by object
    // index + serial, so lookups keyed on it stay correct after the owner actor is destroyed.
    // Default (explicitly-null) when the entity is not part of an editor preview.
    static auto
    TryGet_SelectionOwnerWeak(
        const FCk_Handle& InHandle) -> TWeakObjectPtr<AActor>;

    // Stamp the owner on a preview entity — mirrors Request_SetupEntityWithContextOwner: a
    // re-setup with the same owner is a no-op, a different owner is an ensure. Called by
    // Request_SetupEntityWithLifetimeOwner so every lifetime-descendant inherits the fragment
    // at creation, and by preview owners (the entity spawner) to stamp the root BEFORE any of
    // its descendants are created.
    static auto
    Request_SetupEntityWithEditorSelectionOwner(
        FCk_Handle& InNewEntity,
        const TWeakObjectPtr<AActor>& InOwnerActor) -> void;

    // Editor-world helper actors that host preview visuals for an owner (per-owner ISM/ISKM
    // renderers, the shared selection-proxy host) register here. The owner's
    // PushSelectionToProxies override calls PushOwnerSelectionToProxies to forward the selection
    // highlight to them — they are not attached to the owner, so the engine's attached-actor
    // propagation cannot reach them.
    static auto
    RegisterProxyActor(
        AActor* InOwnerActor,
        AActor* InProxyActor) -> void;

    static auto
    PushOwnerSelectionToProxies(
        const AActor* InOwnerActor) -> void;

    // One-call convenience for loose-component creators (hosted UActorComponents, world-space
    // widgets, opted-in PMG shapes): resolves the entity's selection owner and returns its
    // per-owner host actor (UCk_EditorEcsWorld_Subsystem_UE::Get_SelectionProxyHostActor).
    // Returns nullptr outside editor worlds or when no owner is stamped — callers then fall
    // back to their non-preview hosting.
    static auto
    TryGet_SelectionProxyHostActor(
        const UWorld* InWorld,
        const FCk_Handle& InHandle) -> AActor*;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
