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
    // Stamped on the editor subsystem's transient entity and cascaded to every descendant so
    // editor-only processors can filter their views to authoring-time entities.
    CK_DEFINE_ECS_TAG(FTag_EditorOnlyEntity);
}

// View-filter shortcut — modelled on CK_IF_END_PLAY.
#define CK_IF_EDITOR_ONLY_ENTITY ck::FTag_EditorOnlyEntity

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

    TArray<TOptional<ck::FProcessorScheduler>> _Schedulers;

    bool _PendingRebuildGraph = false;
    FDelegateHandle _OnEndFrameHandle;

    bool _PendingRedraw = false;
};

// --------------------------------------------------------------------------------------------------------------------
