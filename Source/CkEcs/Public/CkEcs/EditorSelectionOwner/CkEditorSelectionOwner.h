#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;
class UWorld;

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
namespace ck
{
    // Stamped on the root entity of an editor-world preview spawned on behalf of a placed actor
    // (e.g. ACk_EntitySpawner_UE's editor entity). Editor-world visuals created for the entity or
    // its lifetime-descendants (ISM instances, hosted scene components) resolve this to host
    // themselves on a per-owner proxy actor, so a viewport click on the visual redirects selection
    // to the placed actor via the engine's selection-parent mechanism (AActor::GetSelectionParent —
    // the same pattern ALevelInstanceEditorInstanceActor uses).
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

    public:
        CK_PROPERTY_GET(_OwnerActor);
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::editor_selection_owner
{
    // Stamp the owner on a (root) editor-preview entity. Descendant entities resolve it via TryGet.
    CKECS_API auto
    Set(
        FCk_Handle& InHandle,
        AActor* InOwnerActor) -> void;

    // Walks the lifetime-ownership chain upward from InHandle and returns the first stamped owner
    // actor found. Returns nullptr when the entity is not part of an editor preview or the owner
    // actor is gone.
    CKECS_API auto
    TryGet(
        const FCk_Handle& InHandle) -> AActor*;

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
