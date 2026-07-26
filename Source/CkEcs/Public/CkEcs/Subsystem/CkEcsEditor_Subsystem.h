#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"
#include "CkEcs/Scheduler/CkProcessorScheduler.h"
#include "CkEcs/Tag/CkTag.h"

#include <Subsystems/WorldSubsystem.h>

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

    // False during the PIE start/stop transition window, when mutating an editor entity would resolve
    // stale handles against a freed registry and flood the MessageLog. Callers must skip and re-arm.
    auto
    Get_IsEditorEcsMutationSafe() const -> bool;

#if WITH_EDITOR
    // Lazily-spawned transient host for InSelectionOwner's editor-preview components. Loose scene
    // components outer here so a viewport click on them redirects selection to the owner.
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
    // _Registry below is a non-owning (slot+gen) view bound to this owned registry.
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
    // Weak on both sides — the world owns the actors, and weak keys stay addressable while an owner
    // is mid-undo or already destroyed.
    TMap<TWeakObjectPtr<const AActor>, TWeakObjectPtr<ACk_EditorSelectionProxyHost_Actor_UE>> _SelectionProxyHostActors;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
