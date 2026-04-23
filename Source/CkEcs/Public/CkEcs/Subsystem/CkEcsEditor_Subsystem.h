#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Scheduler/CkProcessorScheduler.h"
#include "CkEcs/Tag/CkTag.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkEcsEditor_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Tag automatically stamped on every entity spawned through the editor-world subsystem.
    // Editor-only processors filter their view on this tag so they only touch entities that
    // belong to an authoring session (i.e. placed in a level but not in PIE).
    CK_DEFINE_ECS_TAG(FTag_EditorOnlyEntity);
}

// Cosmetic view-filter shortcut — expands to ck::FTag_EditorOnlyEntity. Modelled on CK_IF_END_PLAY
// (CkEntityLifetime_Fragment.h) so a processor header reads as a sentence:
//
//     class FProcessor_X_EditorTime : public ck::TProcessor<
//         FProcessor_X_EditorTime,
//         FCk_Handle_X,
//         CK_IF_EDITOR_ONLY_ENTITY,   // <-- "only for editor entities"
//         CK_IGNORE_PENDING_KILL> { ... };
//
// The macro is a rename only — it does not interact with ECk_ProcessorWorldTypeRequirement, which
// must still be declared explicitly on the processor class.
#define CK_IF_EDITOR_ONLY_ENTITY ck::FTag_EditorOnlyEntity

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_EditorEcsWorld")
class CKECS_API UCk_EditorEcsWorld_Subsystem_UE : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EditorEcsWorld_Subsystem_UE);

public:
    // ---- UWorldSubsystem ----
    auto
    ShouldCreateSubsystem(
        UObject* InOuter) const -> bool override;

    auto
    Initialize(
        FSubsystemCollectionBase& Collection) -> void override;

    auto
    Deinitialize() -> void override;

    // ---- UTickableWorldSubsystem ----
    auto
    Tick(
        float DeltaTime) -> void override;

    auto
    GetStatId() const -> TStatId override;

    auto
    IsTickableInEditor() const -> bool override { return true; }

public:
    // Spawns an entity instance of InScriptArchetype in this editor subsystem's registry,
    // stamps ck::FTag_EditorOnlyEntity on the new entity, and returns the new entity handle
    // once construction completes. The scheduler runs in-editor ticks — the returned handle
    // may still be in the Construction phase on the same frame.
    auto
    Request_SpawnEditorEntity(
        UCk_EntityScript_UE* InScriptArchetype) -> FCk_Handle;

    // Destroys an editor entity (and its lifetime dependents) in this subsystem's registry.
    auto
    Request_DestroyEditorEntity(
        FCk_Handle& InHandle) -> void;

    // Request a full teardown and rebuild of the editor-world processor graph at the end of the
    // current frame. Idempotent within a frame. Used when Blueprint reinstancing invalidates the
    // live EntityScripts held by our editor entities.
    auto
    Request_RebuildProcessorGraph() -> void;

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
    FCk_Registry _Registry;

    UPROPERTY(Transient, meta = (AllowPrivateAccess = true))
    FCk_Handle _TransientEntity;

    // One scheduler per tick group. Editor ticks run them in tick-group iteration order each frame;
    // cross-partition edges were dropped at graph build time so the order between partitions does
    // not affect correctness.
    TArray<TOptional<ck::FProcessorScheduler>> _Schedulers;

    bool _PendingRebuildGraph = false;
    FDelegateHandle _OnEndFrameHandle;
};

// --------------------------------------------------------------------------------------------------------------------
