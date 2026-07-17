#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"
#include "CkEcs/Scheduler/CkProcessorScheduler.h"
#include "CkEcs/Tag/CkTag.h"

#include <Subsystems/WorldSubsystem.h>
#include <UObject/ObjectKey.h>

#include "CkEcsEditor_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;
class ACk_EditorSelectionProxyHost_Actor_UE;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_EditorEcsWorld")
class CKECS_API UCk_EditorEcsWorld_Subsystem_UE : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EditorEcsWorld_Subsystem_UE);

public:
    auto
    ShouldCreateSubsystem(
        UObject* InOuter) const -> bool override;

    auto
    Initialize(
        FSubsystemCollectionBase& Collection) -> void override;

    auto
    Deinitialize() -> void override;

    auto
    Tick(
        float DeltaTime) -> void override;

    auto
    GetStatId() const -> TStatId override;

    auto
    IsTickableInEditor() const -> bool override { return true; }

public:
    auto
    Request_SpawnEditorEntity(
        UCk_EntityScript_UE* InScriptArchetype) -> FCk_Handle;

    auto
    Request_DestroyEditorEntity(
        FCk_Handle& InHandle) -> void;

    auto
    Request_RebuildProcessorGraph() -> void;

    auto
    Request_Redraw() -> void;

    // False during the PIE start/stop transition window: either PIE is active (GEditor->PlayWorld
    // valid) or the editor ECS registry has been torn down / not yet built (the transient entity
    // resolves invalid). Mutating editor entities in this window — spawning (Set_DebugName Has-query
    // + deferred archetype spawn) or destroying — resolves stale handles against a freed registry
    // and floods the MessageLog. Callers must skip and re-arm. Mirrors the scheduler-tick guard in
    // Tick().
    auto
    Get_IsEditorEcsMutationSafe() const -> bool;

#if WITH_EDITOR
    // Lazily-spawned transient host actor for InSelectionOwner's editor-preview components (see
    // ck::FFragment_EditorSelectionOwner). Loose scene components (hosted UActorComponents,
    // world-space widgets, ...) outer here so a viewport click on them redirects selection to
    // the owner — one host per owner, shared by every module that creates preview visuals.
    auto
    Get_SelectionProxyHostActor(
        AActor* InSelectionOwner) -> AActor*;
#endif

public:
    CK_PROPERTY_GET(_Registry);
    CK_PROPERTY_GET_NON_CONST(_Registry);
    CK_PROPERTY_GET(_TransientEntity);

private:
    auto
    DoBuildGraphAndSchedulers() -> void;

    auto
    DoTeardownSchedulers() -> void;

    auto
    OnEndFrame_DoRebuild() -> void;

private:
    // Owns the underlying entt registry. Slot is registered with
    // ck::registry_table on Initialize; freed on Deinitialize. _Registry below
    // is a non-owning view (slot+gen) bound to this owned registry.
    TUniquePtr<ck::registry_table::EnttRegistryType> _OwnedRegistry;
    FCk_Registry _Registry;

    UPROPERTY(Transient, meta = (AllowPrivateAccess = true))
    FCk_Handle _TransientEntity;

    TArray<TOptional<ck::FProcessorScheduler>> _Schedulers;

    bool _PendingRebuildGraph = false;
    FDelegateHandle _OnEndFrameHandle;

    bool _PendingRedraw = false;

#if WITH_EDITORONLY_DATA
private:
    // Weak values: the actors are owned by the world; entries self-heal via validity checks.
    // Keyed by FObjectKey so entries stay addressable even while an owner is mid-undo.
    TMap<FObjectKey, TWeakObjectPtr<ACk_EditorSelectionProxyHost_Actor_UE>> _SelectionProxyHostActors;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
