#include "CkEntityVisualizer_EditorSubsystem.h"

#if WITH_EDITOR
#include "CkEntityVisualizer/CkEntityVisualizer_Fragment.h"
#include "CkEntityVisualizer/Settings/CkEntityVisualizer_Settings.h"
#include "CkEntityVisualizer/CkEntityVisualizer_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Fragment.h"
#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Subsystem/CkEcsEditor_Subsystem.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"
#include "CkSpatialQuery/Probe/CkProbe_Utils.h"
#include "CkShapes/Box/CkShapeBox_Fragment.h"
#include "CkShapes/Capsule/CkShapeCapsule_Fragment.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Fragment.h"
#include "CkShapes/CkShapes_Utils.h"
#include "CkShapes/Sphere/CkShapeSphere_Fragment.h"

#include <GameFramework/Actor.h>
#include <Async/Async.h>
#include <HAL/IConsoleManager.h>
#include <Misc/CoreDelegates.h>
#endif

#if WITH_EDITOR
namespace ck_entity_visualizer_editor_subsystem
{
    TAutoConsoleVariable<int32> CVar_EnableTransformGizmos{
        TEXT("ck.EntityVisualizer.TransformGizmos"),
        1,
        TEXT("Create retained transform gizmos for entities allowed by ck.EntityVisualizer.VisibilityMode."),
        ECVF_Default};

    TAutoConsoleVariable<int32> CVar_EnableProbePreviews{
        TEXT("ck.EntityVisualizer.ProbePreviews"),
        1,
        TEXT("Create retained CkPmg probe previews for entities allowed by ck.EntityVisualizer.VisibilityMode."),
        ECVF_Default};

    TAutoConsoleVariable<int32> CVar_UsePmgTransformGizmos{
        TEXT("ck.EntityVisualizer.UsePmgTransformGizmos"),
        1,
        TEXT("Use retained CkPmg arrows in Selected Only mode. All mode always uses CkIsm."),
        ECVF_Default};
}
#endif

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
#if WITH_EDITOR
    const auto* World = Cast<UWorld>(InOuter);
    return ck::IsValid(World) && World->WorldType == EWorldType::Editor;
#else
    return false;
#endif
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

#if WITH_EDITOR
    _IsInitialized = true;
    _SelectionRefreshHandle = UCk_Utils_EditorSelectionOwner_UE::Get_OnRefreshRequested().AddUObject(
        this, &UCk_EntityVisualizer_EditorSubsystem_UE::QueueOwnerRefresh);
    _TransformAddedHandle = UCk_Utils_Transform_UE::Get_OnAdded().AddUObject(
        this, &UCk_EntityVisualizer_EditorSubsystem_UE::OnTransformAdded);
    _ShapeDimensionsChangedHandle = UCk_Utils_Shapes_UE::Get_OnDimensionsChanged().AddUObject(
        this, &UCk_EntityVisualizer_EditorSubsystem_UE::OnShapeDimensionsChanged);
    _ProbeDebugInfoChangedHandle = UCk_Utils_Probe_UE::Get_OnDebugInfoChanged().AddUObject(
        this, &UCk_EntityVisualizer_EditorSubsystem_UE::OnProbeDebugInfoChanged);

    BindVisualizerCVar(TEXT("ck.EntityVisualizer.VisibilityMode"));
    BindVisualizerCVar(TEXT("ck.EntityVisualizer.TransformGizmos"));
    BindVisualizerCVar(TEXT("ck.EntityVisualizer.ProbePreviews"));
    BindVisualizerCVar(TEXT("ck.EntityVisualizer.UsePmgTransformGizmos"));
    QueueRefreshAllOwners();
#endif
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    Deinitialize()
    -> void
{
#if WITH_EDITOR
    _IsInitialized = false;

    if (_SelectionRefreshHandle.IsValid())
    {
        UCk_Utils_EditorSelectionOwner_UE::Get_OnRefreshRequested().Remove(_SelectionRefreshHandle);
        _SelectionRefreshHandle.Reset();
    }

    if (_TransformAddedHandle.IsValid())
    {
        UCk_Utils_Transform_UE::Get_OnAdded().Remove(_TransformAddedHandle);
        _TransformAddedHandle.Reset();
    }

    if (_ShapeDimensionsChangedHandle.IsValid())
    {
        UCk_Utils_Shapes_UE::Get_OnDimensionsChanged().Remove(_ShapeDimensionsChangedHandle);
        _ShapeDimensionsChangedHandle.Reset();
    }

    if (_ProbeDebugInfoChangedHandle.IsValid())
    {
        UCk_Utils_Probe_UE::Get_OnDebugInfoChanged().Remove(_ProbeDebugInfoChangedHandle);
        _ProbeDebugInfoChangedHandle.Reset();
    }

    if (_EndFrameHandle.IsValid())
    {
        FCoreDelegates::OnEndFrame.Remove(_EndFrameHandle);
        _EndFrameHandle.Reset();
    }

    for (const auto& [CVar, Handle] : _CVarBindings)
    {
        if (CVar != nullptr && Handle.IsValid())
        { CVar->OnChangedDelegate().Remove(Handle); }
    }
    _CVarBindings.Reset();

    _QueuedOwners.Reset();
    _AddedTransformCandidates.Reset();
    _RefreshOwnerlessRequested = false;
    _VisualsByOwner.Reset();
    _OwnerlessVisuals.Reset();
#endif

    Super::Deinitialize();
}

#if WITH_EDITOR
auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    QueueOwnerRefresh(
        const AActor* InOwnerActor)
    -> void
{
    if (NOT _IsInitialized)
    { return; }

    if (ck::Is_NOT_Valid(InOwnerActor, ck::IsValid_Policy_NullptrOnly{}) || InOwnerActor->GetWorld() != GetWorld())
    { return; }

    _QueuedOwners.Add(InOwnerActor);
    ScheduleRefresh();
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    QueueRefreshAllOwners()
    -> void
{
    if (NOT _IsInitialized)
    { return; }

    _RefreshAllOwners = true;
    ScheduleRefresh();
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    ScheduleRefresh()
    -> void
{
    if (NOT _IsInitialized)
    { return; }

    if (_EndFrameHandle.IsValid())
    { return; }

    _EndFrameHandle = FCoreDelegates::OnEndFrame.AddWeakLambda(this, [this]()
    {
        OnEndFrame_Refresh();
    });
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    OnEndFrame_Refresh()
    -> void
{
    if (NOT _IsInitialized)
    { return; }

    if (_EndFrameHandle.IsValid())
    {
        FCoreDelegates::OnEndFrame.Remove(_EndFrameHandle);
        _EndFrameHandle.Reset();
    }

    auto* EditorSubsystem = GetWorld()->GetSubsystem<UCk_EditorEcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EditorSubsystem) || NOT EditorSubsystem->Get_IsEditorEcsMutationSafe())
    {
        ScheduleRefresh();
        return;
    }

    for (const auto& Candidate : _AddedTransformCandidates)
    {
        if (ck::IsValid(Candidate) && NOT Candidate.Has<ck::FTag_EntityVisualizer_Visual>())
        {
            _RefreshAllOwners = true;
            break;
        }
    }
    _AddedTransformCandidates.Reset();

    const auto& Registry = EditorSubsystem->Get_Registry();
    auto RefreshOwnerlessVisuals = _RefreshOwnerlessRequested;
    _RefreshOwnerlessRequested = false;
    if (_RefreshAllOwners)
    {
        for (const auto& [Owner, Visuals] : _VisualsByOwner)
        { _QueuedOwners.Add(Owner); }

        Registry.View<ck::FFragment_EditorSelectionOwner>().ForEach(
            [this](const FCk_Entity /*InEntity*/, const ck::FFragment_EditorSelectionOwner& InOwner)
            {
                if (const auto* OwnerActor = InOwner.Get_OwnerActor().Get();
                    ck::IsValid(OwnerActor, ck::IsValid_Policy_NullptrOnly{}))
                { _QueuedOwners.Add(OwnerActor); }
            });
        RefreshOwnerlessVisuals = true;
        _RefreshAllOwners = false;
    }

    const auto OwnersToRefresh = _QueuedOwners.Array();
    _QueuedOwners.Reset();
    for (const auto& Owner : OwnersToRefresh)
    { RefreshOwner(Owner.Get()); }

    if (RefreshOwnerlessVisuals)
    { RefreshOwnerless(); }

    EditorSubsystem->Request_Redraw();
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    OnTransformAdded(
        const FCk_Handle_Transform& InTransform)
    -> void
{
    if (NOT _IsInitialized || ck::Is_NOT_Valid(InTransform))
    { return; }

    const auto* EditorSubsystem = GetWorld()->GetSubsystem<UCk_EditorEcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EditorSubsystem) ||
        InTransform.Get_RegistryView().Get_RegistryHandle() !=
        EditorSubsystem->Get_Registry().Get_RegistryHandle())
    { return; }

    _AddedTransformCandidates.Add(InTransform.ConvertToHandle());
    ScheduleRefresh();
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    OnShapeDimensionsChanged(
        const FCk_Handle& InShape)
    -> void
{
    if (NOT _IsInitialized || ck::Is_NOT_Valid(InShape))
    { return; }

    const auto* EditorSubsystem = GetWorld()->GetSubsystem<UCk_EditorEcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EditorSubsystem) ||
        InShape.Get_RegistryView().Get_RegistryHandle() !=
        EditorSubsystem->Get_Registry().Get_RegistryHandle())
    { return; }

    if (InShape.Has<ck::FFragment_EditorSelectionOwner>())
    {
        const auto* OwnerActor = InShape.Get<ck::FFragment_EditorSelectionOwner>().Get_OwnerActor().Get();
        QueueOwnerRefresh(OwnerActor);
        return;
    }

    _RefreshOwnerlessRequested = true;
    ScheduleRefresh();
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    OnProbeDebugInfoChanged(
        const FCk_Handle_Probe& InProbe)
    -> void
{
    OnShapeDimensionsChanged(InProbe.ConvertToHandle());
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    RefreshOwner(
        const AActor* InOwnerActor)
    -> void
{
    const auto OwnerKey = TWeakObjectPtr<const AActor>{InOwnerActor};
    DestroyVisualsForOwner(OwnerKey);

    if (ck::Is_NOT_Valid(InOwnerActor))
    { return; }

    const auto OwnerIsSelected = InOwnerActor->IsSelected();
    const auto VisibilityMode = ck::entity_visualizer::GetVisibilityMode();
    const auto ShowVisuals = ck::entity_visualizer::ShouldVisualize(
        VisibilityMode, /*InHasSelectionOwner=*/true, OwnerIsSelected);
    if (NOT ShowVisuals)
    { return; }

    auto* EditorSubsystem = GetWorld()->GetSubsystem<UCk_EditorEcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EditorSubsystem))
    { return; }

    const auto& Registry = EditorSubsystem->Get_Registry();
    auto Sources = TArray<FCk_Handle>{};
    Registry.View<
        ck::FFragment_Transform,
        ck::FFragment_EditorSelectionOwner,
        ck::TExclude<ck::FTag_EntityVisualizer_Visual>,
        ck::TExclude<ck::FTag_DestroyEntity_EndPlay>,
        ck::TExclude<ck::FTag_DestroyEntity_Teardown>,
        ck::TExclude<ck::FTag_DestroyEntity_Await>,
        ck::TExclude<ck::FTag_DestroyEntity_Finalize>>().ForEach(
        [&Sources, &Registry, InOwnerActor](
            const FCk_Entity InEntity,
            const ck::FFragment_Transform& /*InTransform*/,
            const ck::FFragment_EditorSelectionOwner& InOwner)
        {
            if (InOwner.Get_OwnerActor().Get() == InOwnerActor)
            { Sources.Emplace(InEntity, Registry.Get_RegistryHandle()); }
        });

    // Selected, low-cardinality gizmos prefer CkPmg. Preview-all uses the shared ISM path so
    // thousands of sources do not feed persistent debug geometry through the line batcher.
    const auto UseIsmTransformGizmos =
        VisibilityMode == ECk_EntityVisualizer_VisibilityMode::All ||
        ck_entity_visualizer_editor_subsystem::CVar_UsePmgTransformGizmos.GetValueOnGameThread() == 0;
    auto NewVisuals = CreateVisualsForSources(
        MoveTemp(Sources), ShowVisuals, ShowVisuals, UseIsmTransformGizmos);

    if (NOT NewVisuals.IsEmpty())
    { _VisualsByOwner.Add(OwnerKey, MoveTemp(NewVisuals)); }
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    RefreshOwnerless()
    -> void
{
    DestroyVisuals(_OwnerlessVisuals);

    const auto ShowVisuals = ck::entity_visualizer::ShouldVisualize(
        ck::entity_visualizer::GetVisibilityMode(),
        /*InHasSelectionOwner=*/false,
        /*InSelectionOwnerIsSelected=*/false);
    if (NOT ShowVisuals)
    { return; }

    auto* EditorSubsystem = GetWorld()->GetSubsystem<UCk_EditorEcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EditorSubsystem))
    { return; }

    const auto& Registry = EditorSubsystem->Get_Registry();
    auto Sources = TArray<FCk_Handle>{};
    Registry.View<
        ck::FFragment_Transform,
        ck::TExclude<ck::FFragment_EditorSelectionOwner>,
        ck::TExclude<ck::FTag_EntityVisualizer_Visual>,
        ck::TExclude<ck::FTag_DestroyEntity_EndPlay>,
        ck::TExclude<ck::FTag_DestroyEntity_Teardown>,
        ck::TExclude<ck::FTag_DestroyEntity_Await>,
        ck::TExclude<ck::FTag_DestroyEntity_Finalize>>().ForEach(
        [&Sources, &Registry](const FCk_Entity InEntity, const ck::FFragment_Transform& /*InTransform*/)
        {
            Sources.Emplace(InEntity, Registry.Get_RegistryHandle());
        });

    // Ownerless entities have no selectable actor. They are therefore only visible through an
    // explicit preview-all mode, where the high-count transform path must be ISM-backed.
    _OwnerlessVisuals = CreateVisualsForSources(
        MoveTemp(Sources), ShowVisuals, ShowVisuals, true);
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    CreateVisualsForSources(
        TArray<FCk_Handle> InSources,
        bool InShowTransformGizmos,
        bool InShowProbePreviews,
        bool InUseIsmTransformGizmos)
    -> TArray<FCk_Handle>
{
    auto NewVisuals = TArray<FCk_Handle>{};
    const auto CreateTransformGizmos = InShowTransformGizmos &&
        ck_entity_visualizer_editor_subsystem::CVar_EnableTransformGizmos.GetValueOnGameThread() != 0;
    const auto CreateProbePreviews = InShowProbePreviews &&
        ck_entity_visualizer_editor_subsystem::CVar_EnableProbePreviews.GetValueOnGameThread() != 0;

    for (auto& Source : InSources)
    {
        auto SourceTransform = UCk_Utils_Transform_UE::Cast(Source);
        if (ck::Is_NOT_Valid(SourceTransform))
        { continue; }

        if (CreateTransformGizmos)
        {
            if (NOT InUseIsmTransformGizmos)
            {
                for (const auto& Visual : UCk_Utils_EntityVisualizer_UE::Create_TransformGizmo_Pmg(SourceTransform))
                { NewVisuals.Add(Visual.ConvertToHandle()); }
            }
            else
            {
                for (const auto& Visual : UCk_Utils_EntityVisualizer_UE::Create_TransformGizmo_Ism(SourceTransform))
                { NewVisuals.Add(Visual.ConvertToHandle()); }
            }
        }

        if (NOT CreateProbePreviews ||
            NOT Source.Has<ck::FFragment_Probe_DebugInfo>())
        { continue; }

        const auto& DebugInfo = Source.Get<ck::FFragment_Probe_DebugInfo>();
        auto ProbeVisual = FCk_Handle_Pmg_DebugShape{};
        if (Source.Has<ck::FFragment_ShapeBox_Current>())
        {
            ProbeVisual = UCk_Utils_EntityVisualizer_UE::Create_ProbePreview_Pmg(
                SourceTransform, Source.Get<ck::FFragment_ShapeBox_Current>(), DebugInfo);
        }
        else if (Source.Has<ck::FFragment_ShapeSphere_Current>())
        {
            ProbeVisual = UCk_Utils_EntityVisualizer_UE::Create_ProbePreview_Pmg(
                SourceTransform, Source.Get<ck::FFragment_ShapeSphere_Current>(), DebugInfo);
        }
        else if (Source.Has<ck::FFragment_ShapeCapsule_Current>())
        {
            ProbeVisual = UCk_Utils_EntityVisualizer_UE::Create_ProbePreview_Pmg(
                SourceTransform, Source.Get<ck::FFragment_ShapeCapsule_Current>(), DebugInfo);
        }
        else if (Source.Has<ck::FFragment_ShapeCylinder_Current>())
        {
            ProbeVisual = UCk_Utils_EntityVisualizer_UE::Create_ProbePreview_Pmg(
                SourceTransform, Source.Get<ck::FFragment_ShapeCylinder_Current>(), DebugInfo);
        }

        if (ck::IsValid(ProbeVisual))
        { NewVisuals.Add(ProbeVisual.ConvertToHandle()); }
    }

    return NewVisuals;
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    DestroyVisualsForOwner(
        const TWeakObjectPtr<const AActor>& InOwnerKey)
    -> void
{
    auto* Visuals = _VisualsByOwner.Find(InOwnerKey);
    if (Visuals == nullptr)
    { return; }

    DestroyVisuals(*Visuals);
    _VisualsByOwner.Remove(InOwnerKey);
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    DestroyVisuals(
        TArray<FCk_Handle>& InVisuals)
    -> void
{
    for (auto& Visual : InVisuals)
    {
        if (ck::IsValid(Visual))
        { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Visual); }
    }
    InVisuals.Reset();
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    BindVisualizerCVar(
        const TCHAR* InName)
    -> void
{
    auto* CVar = IConsoleManager::Get().FindConsoleVariable(InName);
    if (CVar == nullptr)
    { return; }

    const auto Handle = CVar->OnChangedDelegate().AddUObject(
        this, &UCk_EntityVisualizer_EditorSubsystem_UE::OnVisualizerCVarChanged);
    _CVarBindings.Emplace(CVar, Handle);
}

auto
    UCk_EntityVisualizer_EditorSubsystem_UE::
    OnVisualizerCVarChanged(
        IConsoleVariable* /*InConsoleVariable*/)
    -> void
{
    if (NOT _IsInitialized)
    { return; }

    const auto WeakThis = TWeakObjectPtr<UCk_EntityVisualizer_EditorSubsystem_UE>{this};
    AsyncTask(ENamedThreads::GameThread, [WeakThis]()
    {
        if (WeakThis.IsValid() && WeakThis->_IsInitialized)
        { WeakThis->QueueRefreshAllOwners(); }
    });
}
#endif
