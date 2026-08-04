#include "CkVoxelNavEditor/Preview/CkVoxelNavPreview_EditorSubsystem.h"

#include "CkVoxelNavEditor/Preview/CkVoxelNavPreview_CookedBackend.h"

#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJolt/StaticWorld/CkJoltBakeExtraction.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_CookedQuery.h"
#include "CkJoltEditor/Cook/CkJoltCook_WorldCooker.h"

#include "CkVoxelNav/Authoring/CkVoxelNavVolume_Actor.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Build.h"

#include <Editor.h>
#include <Containers/Ticker.h>
#include <Engine/Level.h>
#include <EngineUtils.h>
#include <Engine/World.h>
#include <HAL/PlatformTime.h>
#include <LevelEditor.h>
#include <Misc/CoreDelegates.h>
#include <Modules/ModuleManager.h>
#include <UObject/ObjectKey.h>
#include <UObject/UObjectGlobals.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_preview_editor_subsystem
{
    constexpr auto RefreshDebounceSeconds = 0.5;

    auto
    Make_SourceFingerprint(
        const ACk_VoxelNavVolume_UE& InActor,
        const ck::voxelnav::FDebugSnapshotBuildParams& InParams) -> uint64
    {
        auto Hash = GetTypeHash(InActor.GetPathName());
        Hash = HashCombineFast(Hash, GetTypeHash(InActor.GetActorTransform().ToString()));
        Hash = HashCombineFast(Hash, GetTypeHash(InActor.Build_ParamsData().Get_VolumeBounds()));
        Hash = HashCombineFast(Hash, GetTypeHash(InActor.Build_ParamsData().Get_FinestCellSizeUu()));
        Hash = HashCombineFast(Hash, GetTypeHash(InActor.Build_ParamsData().Get_ClearanceUu()));
        Hash = HashCombineFast(Hash, GetTypeHash(static_cast<uint8>(InParams._RequestedLayers)));
        Hash = HashCombineFast(Hash, GetTypeHash(InParams._MaxCellsPerLayer));
        Hash = HashCombineFast(Hash, GetTypeHash(InParams._MinOctreeLayer));
        Hash = HashCombineFast(Hash, GetTypeHash(InParams._MaxOctreeLayer));
        Hash = HashCombineFast(Hash, GetTypeHash(InParams._ClipBounds));
        return Hash;
    }

    auto
    Map_QueryStatus(
        ck::jolt::ECk_Jolt_CookedWorldQueryLoadStatus InStatus) -> ck::voxelnav::EDebugSnapshotStatus
    {
        using ck::jolt::ECk_Jolt_CookedWorldQueryLoadStatus;
        using ck::voxelnav::EDebugSnapshotStatus;

        switch (InStatus)
        {
        case ECk_Jolt_CookedWorldQueryLoadStatus::Ready:
            return EDebugSnapshotStatus::Building;
        case ECk_Jolt_CookedWorldQueryLoadStatus::MissingIndex:
        case ECk_Jolt_CookedWorldQueryLoadStatus::MissingCell:
            return EDebugSnapshotStatus::MissingCook;
        case ECk_Jolt_CookedWorldQueryLoadStatus::StaleIndex:
        case ECk_Jolt_CookedWorldQueryLoadStatus::StaleCell:
        case ECk_Jolt_CookedWorldQueryLoadStatus::StaleActor:
            return EDebugSnapshotStatus::StaleCook;
        default:
            return EDebugSnapshotStatus::Failed;
        }
    }

    auto Build_CurrentActorRuntimeHashes(UWorld& InWorld) -> TMap<FName, uint64>
    {
        auto Result = TMap<FName, uint64>{};
        for (const auto& Level : InWorld.GetLevels())
        {
            if (Level == nullptr)
            { continue; }

            for (const auto& Actor : Level->Actors)
            {
                if (Actor == nullptr)
                { continue; }

                Result.Add(Actor->GetFName(), ck::jolt::bake::ComputeRuntimeCheckHash(*Actor));
            }
        }
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------

struct UCk_VoxelNavPreview_EditorSubsystem_UE::FImpl
{
    struct FVolumeState
    {
        TWeakObjectPtr<ACk_VoxelNavVolume_UE> _Actor;
        TUniquePtr<ck::jolt::FCk_Jolt_CookedWorldQuery> _Query;
        TUniquePtr<ck::voxelnav_editor::FCookedGeometryBackend> _Backend;
        ck::voxelnav::FBuildState _Build;
        ck::voxelnav::FBuildParams _BuildParams;
        ck::voxelnav::FDebugSnapshotBuildParams _SnapshotParams;
        ck::voxelnav::FDebugSnapshotCache _Cache;
        ck::voxelnav::EDebugSnapshotStatus _Status = ck::voxelnav::EDebugSnapshotStatus::RuntimeOnly;
        FString _StatusMessage;
        uint64 _Fingerprint = 0;
        int32 _SourceEpoch = 0;
        bool _BuildActive = false;
    };

    TMap<FObjectKey, TUniquePtr<FVolumeState>> _States;
    FTSTicker::FDelegateHandle _TickerHandle;
    FDelegateHandle _MapChangedHandle;
    FDelegateHandle _PropertyChangedHandle;
    ck::voxelnav::FDebugSnapshotBuildParams _LastSnapshotParams;
    TSharedPtr<const TArray<ck::voxelnav::FDebugSnapshot>> _RenderSnapshots =
        MakeShared<const TArray<ck::voxelnav::FDebugSnapshot>>();
    double _RefreshAtSeconds = 0.0;
    bool _RefreshScheduled = true;
};

// --------------------------------------------------------------------------------------------------------------------

UCk_VoxelNavPreview_EditorSubsystem_UE::UCk_VoxelNavPreview_EditorSubsystem_UE() = default;
UCk_VoxelNavPreview_EditorSubsystem_UE::~UCk_VoxelNavPreview_EditorSubsystem_UE() = default;

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);
    _Impl = MakeShared<FImpl>();

    _Impl->_TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UCk_VoxelNavPreview_EditorSubsystem_UE::DoTick));

    auto& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
    _Impl->_MapChangedHandle = LevelEditor.OnMapChanged().AddUObject(
        this, &UCk_VoxelNavPreview_EditorSubsystem_UE::DoOnMapChanged);
    _Impl->_PropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(
        this, &UCk_VoxelNavPreview_EditorSubsystem_UE::DoOnObjectPropertyChanged);

    DoScheduleRefresh();
}

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    Deinitialize()
    -> void
{
    if (_Impl != nullptr)
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_Impl->_TickerHandle);
        FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(_Impl->_PropertyChangedHandle);

        if (_Impl->_MapChangedHandle.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
        {
            auto& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
            LevelEditor.OnMapChanged().Remove(_Impl->_MapChangedHandle);
        }

        DoClear();
        _Impl.Reset();
    }

    Super::Deinitialize();
}

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    Get()
    -> UCk_VoxelNavPreview_EditorSubsystem_UE*
{
    return GEditor != nullptr ? GEditor->GetEditorSubsystem<UCk_VoxelNavPreview_EditorSubsystem_UE>() : nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    Request_RebuildAll(
        const ck::voxelnav::FDebugSnapshotBuildParams& InSnapshotParams)
    -> void
{
    if (_Impl == nullptr || GEditor == nullptr)
    { return; }

    auto* World = GEditor->GetEditorWorldContext().World();
    if (World == nullptr)
    {
        DoClear();
        return;
    }

    _Impl->_LastSnapshotParams = InSnapshotParams;
    _Impl->_RefreshScheduled = false;

    auto VolumeActors = TArray<ACk_VoxelNavVolume_UE*>{};
    for (TActorIterator<ACk_VoxelNavVolume_UE> It{World}; It; ++It)
    { VolumeActors.Emplace(*It); }

    if (VolumeActors.IsEmpty())
    {
        _Impl->_States.Empty();
        DoPublishRenderSnapshots();
        return;
    }

    const auto CookIsCurrent = FCk_Jolt_WorldCooker::Validate_World(*World)._Success;
    auto PresentKeys = TSet<FObjectKey>{};

    for (auto* VolumeActor : VolumeActors)
    {
        PresentKeys.Add(FObjectKey{VolumeActor});
        DoStartBuild(*VolumeActor, InSnapshotParams, CookIsCurrent);
    }

    for (auto It = _Impl->_States.CreateIterator(); It; ++It)
    {
        if (NOT PresentKeys.Contains(It.Key()))
        { It.RemoveCurrent(); }
    }

    DoPublishRenderSnapshots();
}

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    Request_Rebuild(
        ACk_VoxelNavVolume_UE& InActor,
        const ck::voxelnav::FDebugSnapshotBuildParams& InSnapshotParams)
    -> void
{
    auto* World = InActor.GetWorld();
    DoStartBuild(InActor, InSnapshotParams,
        World != nullptr && FCk_Jolt_WorldCooker::Validate_World(*World)._Success);
    DoPublishRenderSnapshots();
}

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    Get_AvailableVolumes() const
    -> TArray<TWeakObjectPtr<ACk_VoxelNavVolume_UE>>
{
    auto Result = TArray<TWeakObjectPtr<ACk_VoxelNavVolume_UE>>{};
    if (_Impl == nullptr)
    { return Result; }

    Result.Reserve(_Impl->_States.Num());
    for (const auto& Pair : _Impl->_States)
    {
        if (Pair.Value->_Actor.IsValid())
        { Result.Emplace(Pair.Value->_Actor); }
    }
    return Result;
}

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    TryGet_Snapshot(
        const ACk_VoxelNavVolume_UE& InActor,
        ck::voxelnav::FDebugSnapshot& OutSnapshot) const
    -> bool
{
    if (_Impl == nullptr)
    { return false; }

    const auto* StatePtr = _Impl->_States.Find(FObjectKey{&InActor});
    if (StatePtr == nullptr)
    { return false; }

    const auto& State = **StatePtr;
    const auto Cached = State._Cache.Get_SnapshotCopy();
    OutSnapshot = Cached.IsSet() ? *Cached : ck::voxelnav::FDebugSnapshot{};
    OutSnapshot._Source = ck::voxelnav::EDebugSnapshotSource::EditorPreview;
    OutSnapshot._Status = State._Status;
    OutSnapshot._StatusMessage = State._StatusMessage;
    OutSnapshot._SourceIdentity = InActor.GetPathName();
    OutSnapshot._SourceEpoch = State._SourceEpoch;
    OutSnapshot._SourceFingerprint = State._Fingerprint;
    OutSnapshot._AuthoredBounds = InActor.Get_WorldVolumeBounds();

    if (State._BuildActive)
    {
        OutSnapshot._BuildStage = State._Build.Get_Stage();
        OutSnapshot._BuildStats = State._Build.Get_Stats();
        OutSnapshot._BuildProgress = State._Build.Get_Progress01();
    }
    return true;
}

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    Get_Snapshots() const
    -> TArray<ck::voxelnav::FDebugSnapshot>
{
    auto Result = TArray<ck::voxelnav::FDebugSnapshot>{};
    for (const auto& Actor : Get_AvailableVolumes())
    {
        auto Snapshot = ck::voxelnav::FDebugSnapshot{};
        if (Actor.IsValid() && TryGet_Snapshot(*Actor, Snapshot))
        { Result.Emplace(MoveTemp(Snapshot)); }
    }
    return Result;
}

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    Get_RenderSnapshots() const
    -> TSharedPtr<const TArray<ck::voxelnav::FDebugSnapshot>>
{
    return _Impl != nullptr
        ? _Impl->_RenderSnapshots
        : MakeShared<const TArray<ck::voxelnav::FDebugSnapshot>>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    DoTick(
        float InDeltaSeconds)
    -> bool
{
    if (_Impl == nullptr)
    { return true; }

    if (_Impl->_RefreshScheduled && FPlatformTime::Seconds() >= _Impl->_RefreshAtSeconds)
    { Request_RebuildAll(_Impl->_LastSnapshotParams); }

    auto RenderPublicationChanged = false;
    for (auto& Pair : _Impl->_States)
    {
        auto& State = *Pair.Value;
        if (NOT State._BuildActive || State._Backend == nullptr)
        { continue; }

        auto Budget = ck::voxelnav::FBuildBudget{};
        Budget._MaxOccupancyProbes = 2048;
        Budget._MaxSeconds = 0.004f;
        Request_AdvanceBuild(State._Build, State._BuildParams, *State._Backend, Budget);

        if (NOT State._Build.Get_IsFinished())
        { continue; }

        const auto BuildStats = State._Build.Get_Stats();
        State._BuildActive = false;
        auto Octree = Request_ReleaseBuiltOctree(State._Build);
        if (NOT Octree.IsValid())
        {
            State._Status = ck::voxelnav::EDebugSnapshotStatus::Failed;
            State._StatusMessage = TEXT("VoxelNav editor preview build failed");
            RenderPublicationChanged = true;
            continue;
        }

        auto Snapshot = ck::voxelnav::FDebugSnapshot{};
        Snapshot._Source = ck::voxelnav::EDebugSnapshotSource::EditorPreview;
        Snapshot._Status = ck::voxelnav::EDebugSnapshotStatus::Current;
        Snapshot._SourceIdentity = State._Actor.IsValid() ? State._Actor->GetPathName() : FString{};
        Snapshot._SourceEpoch = ++State._SourceEpoch;
        Snapshot._SourceFingerprint = State._Fingerprint;
        Snapshot._AuthoredBounds = State._BuildParams._VolumeBounds;
        Snapshot._NavigationBounds = Octree->Get_NavigationBounds();
        Snapshot._BuildStage = ECk_VoxelNav_BuildStage::Complete;
        Snapshot._BuildStats = BuildStats;
        Snapshot._BuildProgress = 1.0f;
        Snapshot._IsBuilt = true;
        Append_OctreeDebugSnapshot(*Octree, INDEX_NONE, State._SnapshotParams, Snapshot);

        auto Key = ck::voxelnav::FDebugSnapshotCacheKey{};
        Key._Source = Snapshot._Source;
        Key._Status = Snapshot._Status;
        Key._Identity = Snapshot._SourceIdentity;
        Key._Epoch = Snapshot._SourceEpoch;
        Key._Fingerprint = Snapshot._SourceFingerprint;
        Key._BuildParams = State._SnapshotParams;
        State._Cache.Publish(Key, MoveTemp(Snapshot));
        State._Status = ck::voxelnav::EDebugSnapshotStatus::Current;
        State._StatusMessage = TEXT("Exact cooked-Jolt editor preview is current");
        RenderPublicationChanged = true;
    }

    if (RenderPublicationChanged)
    { DoPublishRenderSnapshots(); }

    return true;
}

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    DoOnMapChanged(
        UWorld* InWorld,
        EMapChangeType InMapChangeType)
    -> void
{
    DoClear();
    DoScheduleRefresh();
}

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    DoOnObjectPropertyChanged(
        UObject* InObject,
        FPropertyChangedEvent& InPropertyChangedEvent)
    -> void
{
    if (InObject != nullptr && InObject->IsA<AActor>())
    {
        if (_Impl != nullptr)
        {
            for (auto& Pair : _Impl->_States)
            {
                Pair.Value->_Status = ck::voxelnav::EDebugSnapshotStatus::StaleCook;
                Pair.Value->_StatusMessage = TEXT("Editor world changed; waiting for exact cooked-Jolt revalidation");
                Pair.Value->_BuildActive = false;
            }
            DoPublishRenderSnapshots();
        }
        DoScheduleRefresh();
    }
}

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    DoScheduleRefresh()
    -> void
{
    if (_Impl == nullptr)
    { return; }

    _Impl->_RefreshScheduled = true;
    _Impl->_RefreshAtSeconds = FPlatformTime::Seconds() + ck_voxelnav_preview_editor_subsystem::RefreshDebounceSeconds;
}

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    DoClear()
    -> void
{
    if (_Impl != nullptr)
    {
        _Impl->_States.Empty();
        DoPublishRenderSnapshots();
    }
}

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    DoPublishRenderSnapshots()
    -> void
{
    if (_Impl == nullptr)
    { return; }

    auto Published = MakeShared<TArray<ck::voxelnav::FDebugSnapshot>>();
    Published->Reserve(_Impl->_States.Num());
    for (const auto& Pair : _Impl->_States)
    {
        const auto& State = *Pair.Value;
        if (NOT State._Actor.IsValid())
        { continue; }

        auto Snapshot = ck::voxelnav::FDebugSnapshot{};
        if (TryGet_Snapshot(*State._Actor, Snapshot))
        { Published->Emplace(MoveTemp(Snapshot)); }
    }

    _Impl->_RenderSnapshots = Published;
    if (GEditor != nullptr)
    { GEditor->RedrawLevelEditingViewports(); }
}

auto
    UCk_VoxelNavPreview_EditorSubsystem_UE::
    DoStartBuild(
        ACk_VoxelNavVolume_UE& InActor,
        const ck::voxelnav::FDebugSnapshotBuildParams& InSnapshotParams,
        bool InCookIsCurrent)
    -> void
{
    if (_Impl == nullptr)
    { return; }

    auto& StatePtr = _Impl->_States.FindOrAdd(FObjectKey{&InActor});
    if (StatePtr == nullptr)
    { StatePtr = MakeUnique<FImpl::FVolumeState>(); }
    auto& State = *StatePtr;
    State._Actor = &InActor;
    State._SnapshotParams = InSnapshotParams;
    State._Fingerprint = ck_voxelnav_preview_editor_subsystem::Make_SourceFingerprint(InActor, InSnapshotParams);
    State._BuildActive = false;

    const auto AuthoredParams = InActor.Build_ParamsData();
    const auto BoundsAreValid = AuthoredParams.Get_VolumeBounds().IsValid != 0;
    const auto CellSizeIsValid = FMath::IsFinite(AuthoredParams.Get_FinestCellSizeUu()) &&
        AuthoredParams.Get_FinestCellSizeUu() >= 4.0f;
    const auto InputIsValid = BoundsAreValid && CellSizeIsValid && InActor.GetWorld() != nullptr;

    if (NOT InputIsValid)
    {
        State._Status = ck::voxelnav::EDebugSnapshotStatus::Failed;
        State._StatusMessage = TEXT("VoxelNav authoring bounds or finest cell size are invalid");
        State._Query.Reset();
        State._Backend.Reset();
        return;
    }

    if (NOT InCookIsCurrent)
    {
        State._Status = ck::voxelnav::EDebugSnapshotStatus::StaleCook;
        State._StatusMessage = TEXT("Cooked Jolt data is missing or stale; cook the current map before previewing");
        State._Query.Reset();
        State._Backend.Reset();
        return;
    }

    auto Query = MakeUnique<ck::jolt::FCk_Jolt_CookedWorldQuery>();
    auto Request = ck::jolt::FCk_Jolt_CookedWorldQueryLoadRequest{};
    Request._CookedDataRootPath = UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath();
    Request._MapPackageName = InActor.GetWorld()->PersistentLevel->GetOutermost()->GetName();
    Request._OptionalBounds = AuthoredParams.Get_VolumeBounds();
    Request._RequireCurrentActorRuntimeHashes = true;
    Request._CurrentActorRuntimeHashes = ck_voxelnav_preview_editor_subsystem::Build_CurrentActorRuntimeHashes(
        *InActor.GetWorld());
    const auto LoadResult = Query->Request_Load(Request);

    if (NOT LoadResult.Get_IsReady())
    {
        State._Status = ck_voxelnav_preview_editor_subsystem::Map_QueryStatus(LoadResult._Status);
        State._StatusMessage = LoadResult._Message;
        State._Query.Reset();
        State._Backend.Reset();
        return;
    }

    State._Query = MoveTemp(Query);
    State._Backend = MakeUnique<ck::voxelnav_editor::FCookedGeometryBackend>(*State._Query);
    State._Build = ck::voxelnav::FBuildState{};
    State._BuildParams = ck::voxelnav::FBuildParams{};
    State._BuildParams._VolumeBounds = AuthoredParams.Get_VolumeBounds();
    State._BuildParams._FinestCellSizeUu = AuthoredParams.Get_FinestCellSizeUu();
    State._BuildParams._ClearanceUu = AuthoredParams.Get_ClearanceUu();
    State._BuildParams._CellMerging = ECk_EnableDisable::Enable;
    State._Status = ck::voxelnav::EDebugSnapshotStatus::Building;
    State._StatusMessage = TEXT("Building exact cooked-Jolt editor preview");
    State._BuildActive = true;
}

// --------------------------------------------------------------------------------------------------------------------
