#pragma once

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
namespace ck
{
    // Stamped (via UCk_Utils_EditorSelectionOwner_UE::Request_SetupEntityWithEditorSelectionOwner)
    // on the root entity of an editor-world preview spawned on behalf of a placed actor (e.g.
    // ACk_EntitySpawner_UE's editor entity), then inherited by every lifetime-descendant at
    // creation — same strategy as ContextOwner (see Request_SetupEntityWithLifetimeOwner).
    // Consumers read it directly off their entity; there is no chain walk. Editor-world visuals
    // (ISM/ISKM instances, hosted scene components, world-space widgets, opted-in PMG shapes) use
    // it to host on a per-owner proxy actor, so a viewport click on the visual redirects selection
    // to the placed actor via the engine's selection-parent mechanism (AActor::GetSelectionParent —
    // the same pattern ALevelInstanceEditorInstanceActor uses).
    //
    // The weak ptr doubles as the per-owner cache identity: TWeakObjectPtr compares and hashes by
    // object index + serial, so lookups keyed on it stay correct even after the owner actor is
    // destroyed (teardown removes preview visuals from the per-owner host that actually holds them).
    struct CKECS_API FFragment_EditorSelectionOwner
    {
    public:
        CK_GENERATED_BODY(FFragment_EditorSelectionOwner);

    public:
        FFragment_EditorSelectionOwner() = default;
        explicit FFragment_EditorSelectionOwner(
            const TWeakObjectPtr<AActor>& InOwnerActor);

    private:
        TWeakObjectPtr<AActor> _OwnerActor;

    public:
        CK_PROPERTY_GET(_OwnerActor);
    };
}
#endif

// --------------------------------------------------------------------------------------------------------------------
