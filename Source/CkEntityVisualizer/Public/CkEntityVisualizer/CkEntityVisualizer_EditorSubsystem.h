#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkEntityVisualizer_EditorSubsystem.generated.h"

class AActor;
class IConsoleVariable;
struct FCk_Handle_Transform;
struct FCk_Handle_Probe;

UCLASS(DisplayName="CkSubsystem_EntityVisualizerEditor")
class CKENTITYVISUALIZER_API UCk_EntityVisualizer_EditorSubsystem_UE : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityVisualizer_EditorSubsystem_UE);

public:
    auto ShouldCreateSubsystem(UObject* InOuter) const -> bool override;
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;

private:
#if WITH_EDITOR
    auto QueueOwnerRefresh(const AActor* InOwnerActor) -> void;
    auto QueueRefreshAllOwners() -> void;
    auto ScheduleRefresh() -> void;
    auto OnEndFrame_Refresh() -> void;
    auto OnVisualizerCVarChanged(IConsoleVariable* InConsoleVariable) -> void;
    auto OnTransformAdded(const FCk_Handle_Transform& InTransform) -> void;
    auto OnShapeDimensionsChanged(const FCk_Handle& InShape) -> void;
    auto OnProbeDebugInfoChanged(const FCk_Handle_Probe& InProbe) -> void;
    auto RefreshOwner(const AActor* InOwnerActor) -> void;
    auto RefreshOwnerless() -> void;
    auto CreateVisualsForSources(
        TArray<FCk_Handle> InSources,
        bool InShowTransformGizmos,
        bool InShowProbePreviews,
        bool InUseIsmTransformGizmos) -> TArray<FCk_Handle>;
    auto DestroyVisualsForOwner(const TWeakObjectPtr<const AActor>& InOwnerKey) -> void;
    static auto DestroyVisuals(TArray<FCk_Handle>& InVisuals) -> void;
    auto BindVisualizerCVar(const TCHAR* InName) -> void;

private:
    TSet<TWeakObjectPtr<const AActor>> _QueuedOwners;
    TArray<FCk_Handle> _AddedTransformCandidates;
    TMap<TWeakObjectPtr<const AActor>, TArray<FCk_Handle>> _VisualsByOwner;
    TArray<FCk_Handle> _OwnerlessVisuals;
    TArray<TPair<IConsoleVariable*, FDelegateHandle>> _CVarBindings;
    bool _RefreshAllOwners = false;
    bool _RefreshOwnerlessRequested = false;
    FDelegateHandle _SelectionRefreshHandle;
    FDelegateHandle _TransformAddedHandle;
    FDelegateHandle _ShapeDimensionsChangedHandle;
    FDelegateHandle _ProbeDebugInfoChangedHandle;
    FDelegateHandle _EndFrameHandle;
    bool _IsInitialized = false;
#endif
};
