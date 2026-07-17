#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include <UObject/ObjectKey.h>

// --------------------------------------------------------------------------------------------------------------------

class AActor;
class UWorld;

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
namespace ck
{
    // Stamped (via editor_selection_owner::Set) on the root entity of an editor-world preview
    // spawned on behalf of a placed actor (e.g. ACk_EntitySpawner_UE's editor entity), then
    // inherited by every lifetime-descendant at creation — same strategy as ContextOwner (see
    // Request_SetupEntityWithLifetimeOwner). Consumers read it directly off their entity; there
    // is no chain walk. Editor-world visuals (ISM/ISKM instances, hosted scene components,
    // world-space widgets, opted-in PMG shapes) use it to host on a per-owner proxy actor, so a
    // viewport click on the visual redirects selection to the placed actor via the engine's
    // selection-parent mechanism (AActor::GetSelectionParent — the same pattern
    // ALevelInstanceEditorInstanceActor uses).
    //
    // _OwnerKey duplicates the actor's identity as an FObjectKey: teardown can run after the
    // owner actor is destroyed (weak ptr dead), and per-owner caches keyed by FObjectKey must
    // keep resolving so instances are removed from the component that actually holds them.
    struct CKECS_API FFragment_EditorSelectionOwner
    {
    public:
        CK_GENERATED_BODY(FFragment_EditorSelectionOwner);

    public:
        FFragment_EditorSelectionOwner() = default;
        explicit FFragment_EditorSelectionOwner(
            AActor* InOwnerActor);

    private:
        TWeakObjectPtr<AActor> _OwnerActor;
        FObjectKey _OwnerKey;

    public:
        CK_PROPERTY_GET(_OwnerActor);
        CK_PROPERTY_GET(_OwnerKey);
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::editor_selection_owner
{
    // Stamp the owner on a (root) editor-preview entity BEFORE any of its descendants are
    // created — lifetime-descendants inherit the fragment at creation
    // (Request_SetupEntityWithLifetimeOwner), so consumers read it directly off their entity.
    CKECS_API auto
    Set(
        FCk_Handle& InHandle,
        AActor* InOwnerActor) -> void;

    // The entity's stamped owner actor. Returns nullptr when the entity is not part of an
    // editor preview or the owner actor is gone.
    CKECS_API auto
    TryGet(
        const FCk_Handle& InHandle) -> AActor*;

    // The entity's stamped owner as a cache key that stays valid after the owner actor is
    // destroyed. Returns a default (invalid) key when the entity is not part of an editor preview.
    CKECS_API auto
    TryGet_OwnerKey(
        const FCk_Handle& InHandle) -> FObjectKey;

    // Editor-world helper actors that host preview visuals for an owner (per-owner ISM renderer,
    // per-owner component host) register here. The owner's PushSelectionToProxies override calls
    // PushOwnerSelectionToProxies to forward the selection highlight to them — they are not
    // attached to the owner, so the engine's attached-actor propagation cannot reach them.
    CKECS_API auto
    RegisterProxyActor(
        AActor* InOwnerActor,
        AActor* InProxyActor) -> void;

    CKECS_API auto
    PushOwnerSelectionToProxies(
        const AActor* InOwnerActor) -> void;

    // One-call convenience for loose-component creators (hosted UActorComponents, world-space
    // widgets, ...): resolves the selection owner from the entity's lifetime chain and returns
    // its per-owner host actor (UCk_EditorEcsWorld_Subsystem_UE::Get_SelectionProxyHostActor).
    // Returns nullptr outside editor worlds or when no owner is stamped — callers then fall
    // back to their non-preview hosting.
    CKECS_API auto
    TryGet_SelectionProxyHostActor(
        const UWorld* InWorld,
        const FCk_Handle& InHandle) -> AActor*;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
