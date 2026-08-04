#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkVoxelNav/Debug/CkVoxelNav_DebugSnapshot.h"

#include <EditorSubsystem.h>

#include "CkVoxelNavPreview_EditorSubsystem.generated.h"

class ACk_VoxelNavVolume_UE;
class UWorld;
enum class EMapChangeType : uint8;
struct FPropertyChangedEvent;

// --------------------------------------------------------------------------------------------------------------------

/** Owns exact outside-PIE VoxelNav previews. It discovers placed authoring actors, validates the map's
 *  cooked Jolt data, advances bounded builds on editor ticks, and publishes only immutable snapshots. */
UCLASS(NotBlueprintable, NotBlueprintType, DisplayName = "CkSubsystem_VoxelNavPreview")
class CKVOXELNAVEDITOR_API UCk_VoxelNavPreview_EditorSubsystem_UE : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_VoxelNavPreview_EditorSubsystem_UE);

    UCk_VoxelNavPreview_EditorSubsystem_UE();
    ~UCk_VoxelNavPreview_EditorSubsystem_UE() override;

public:
    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto Deinitialize() -> void override;

    static auto Get() -> UCk_VoxelNavPreview_EditorSubsystem_UE*;

public:
    /** Restarts every placed volume after one whole-map cooked-source validation. Builds are paced on ticks. */
    auto
    Request_RebuildAll(
        const ck::voxelnav::FDebugSnapshotBuildParams& InSnapshotParams = {}) -> void;

    auto
    Request_Rebuild(
        ACk_VoxelNavVolume_UE& InActor,
        const ck::voxelnav::FDebugSnapshotBuildParams& InSnapshotParams = {}) -> void;

    auto Get_AvailableVolumes() const -> TArray<TWeakObjectPtr<ACk_VoxelNavVolume_UE>>;

    auto
    TryGet_Snapshot(
        const ACk_VoxelNavVolume_UE& InActor,
        ck::voxelnav::FDebugSnapshot& OutSnapshot) const -> bool;

    auto Get_Snapshots() const -> TArray<ck::voxelnav::FDebugSnapshot>;

    /** Render-facing whole-value publication. The pointed-to array is replaced only when source state changes,
     *  so a viewport render never copies cell arrays or observes a partially updated collection. */
    auto Get_RenderSnapshots() const -> TSharedPtr<const TArray<ck::voxelnav::FDebugSnapshot>>;

private:
    auto DoTick(float InDeltaSeconds) -> bool;

    auto
    DoOnMapChanged(
        UWorld* InWorld,
        EMapChangeType InMapChangeType) -> void;

    auto
    DoOnObjectPropertyChanged(
        UObject* InObject,
        FPropertyChangedEvent& InPropertyChangedEvent) -> void;

    auto DoScheduleRefresh() -> void;
    auto DoClear() -> void;
    auto DoPublishRenderSnapshots() -> void;

    auto
    DoStartBuild(
        ACk_VoxelNavVolume_UE& InActor,
        const ck::voxelnav::FDebugSnapshotBuildParams& InSnapshotParams,
        bool InCookIsCurrent) -> void;

private:
    struct FImpl;
    TSharedPtr<FImpl> _Impl;
};

// --------------------------------------------------------------------------------------------------------------------
