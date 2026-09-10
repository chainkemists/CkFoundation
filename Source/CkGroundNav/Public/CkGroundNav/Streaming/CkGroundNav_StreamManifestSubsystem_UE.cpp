#include "CkGroundNav/Streaming/CkGroundNav_StreamManifestSubsystem_UE.h"

#include "CkGroundNav/Streaming/CkGroundNav_StreamManifestBinding_UE.h"
#include "CkGroundNav/Cook/CkGroundNav_CookedFieldLoad.h"

#include <Algo/AllOf.h>
#include <Engine/Level.h>
#include <Engine/World.h>
#include <WorldPartition/DataLayer/DataLayerManager.h>

// --------------------------------------------------------------------------------------------------------------------

auto UCk_GroundNav_StreamManifestSubsystem_UE::Initialize(FSubsystemCollectionBase& InCollection) -> void
{
    Super::Initialize(InCollection);
    _LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(
        this, &UCk_GroundNav_StreamManifestSubsystem_UE::DoHandle_LevelAdded);
    _LevelRemovedHandle = FWorldDelegates::LevelRemovedFromWorld.AddUObject(
        this, &UCk_GroundNav_StreamManifestSubsystem_UE::DoHandle_LevelRemoved);

    // A persistent level can already be present when this subsystem is created. Treat it exactly as a
    // later streamed level so startup and World Partition cell ordering share one consumer path.
    if (auto* World = GetWorld(); World != nullptr)
    {
        for (auto* Level : World->GetLevels())
        { DoHandle_LevelAdded(Level, World); }
    }
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::Deinitialize() -> void
{
    FWorldDelegates::LevelAddedToWorld.Remove(_LevelAddedHandle);
    FWorldDelegates::LevelRemovedFromWorld.Remove(_LevelRemovedHandle);

    for (auto& Binding : _Bindings)
    { DoUnload(Binding); }
    _Bindings.Reset();
    _GenerationLedger.Reset();

    Super::Deinitialize();
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::Tick(float /*InDeltaTime*/) -> void
{
    for (auto& Binding : _Bindings)
    {
        if (Binding._Unloading)
        {
            DoUnload(Binding);
            continue;
        }

        if (Binding._Loaded)
        { DoUpdate_DataLayerActivation(Binding); }
        else if (Binding._AwaitingOwner && NOT Binding._TerminalFailure &&
                 NOT Get_IsBlockedByUnloadingSource(Binding) &&
                 Get_AreDataLayersActivated(Binding))
        { DoTry_Load(Binding); }
    }

    // A level-removal failure remains as a tombstone until Unload has actually removed its source.
    // Only then may a subsequently streamed instance of the same authored key load without racing it.
    _Bindings.RemoveAll([](const FBindingState& InState)
    { return InState._Unloading && NOT InState._Loaded; });
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::GetStatId() const -> TStatId
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCk_GroundNav_StreamManifestSubsystem_UE, STATGROUP_Tickables);
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::Get_AreDataLayerRuntimeStatesActivated(
    TConstArrayView<EDataLayerRuntimeState> InStates) -> bool
{
    return Algo::AllOf(InStates, [](const EDataLayerRuntimeState InState)
    { return InState == EDataLayerRuntimeState::Activated; });
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::Get_AreManifestTileCoordsCompatible(
    TConstArrayView<FIntPoint> InBindingCoords,
    TConstArrayView<FIntPoint> InManifestCoords) -> bool
{
    if (InBindingCoords.Num() != InManifestCoords.Num())
    { return false; }

    for (auto Index = 0; Index < InBindingCoords.Num(); ++Index)
    {
        if (InBindingCoords[Index] != InManifestCoords[Index])
        { return false; }
    }
    return true;
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::Try_LoadBoundCookedSourceManifest(
    const UCk_GroundNav_CookedSourceManifest_UE& InManifest,
    TConstArrayView<FIntPoint> InBindingCoords,
    const ck::groundnav::FCk_GroundNav_StreamFieldBundle& InTemplateBundle,
    const uint64 InDefaultFingerprint,
    const TMap<FGameplayTag, uint64>& InVariantFingerprints,
    const int32 InStreamingVolumeId,
    const int32 InPartitionId,
    const ck::groundnav::FCk_GroundNav_DataLayerSelector& InDataLayerSelector,
    TArray<ck::groundnav::FCk_GroundNav_StreamTileTransition>& OutTransitions)
    -> ECk_GroundNav_CookStatus
{
    if (NOT Get_AreManifestTileCoordsCompatible(InBindingCoords, InManifest.Get_TileCoords()))
    { return ECk_GroundNav_CookStatus::StaleCook; }

    return ck::groundnav::Try_LoadCookedSourceManifest(
        InManifest, InTemplateBundle, InDefaultFingerprint, InVariantFingerprints,
        InStreamingVolumeId, InPartitionId, InDataLayerSelector, OutTransitions);
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::ShouldCreateSubsystem(UObject* InOuter) const -> bool
{
    if (NOT Super::ShouldCreateSubsystem(InOuter) || InOuter == nullptr)
    { return false; }

    const auto* World = Cast<UWorld>(InOuter);
    return World != nullptr && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::DoHandle_LevelAdded(ULevel* InLevel, UWorld* InWorld) -> void
{
    if (InWorld != GetWorld() || InLevel == nullptr)
    { return; }

    for (const auto& Actor : InLevel->Actors)
    {
        auto* Binding = Cast<ACk_GroundNav_StreamManifestBinding_UE>(Actor.Get());
        if (Binding == nullptr || _Bindings.ContainsByPredicate([Binding](const FBindingState& InState)
            { return InState._Binding.Get() == Binding; }))
        { continue; }

        auto Selector = ck::groundnav::FCk_GroundNav_DataLayerSelector{};
        if (NOT Binding->TryGet_DataLayerSelector(Selector))
        { continue; }

        auto State = FBindingState{};
        State._Binding = Binding;
        State._Level = InLevel;
        State._Key._VolumeId = ck::groundnav::FCk_GroundNav_VolumeId{Binding->_StreamingVolumeId};
        State._Key._PartitionId = Binding->_PartitionId;
        State._DataLayerSelector = MoveTemp(Selector);
        State._TileCoords = Binding->_TileCoords;

        // A second resident binding for one durable identity is malformed and cannot replace the
        // active source. A binding arriving behind an unloading tombstone is different: retain it
        // as awaiting, and Tick will admit it only after that prior source is actually gone.
        const auto HasResidentIdentity = _Bindings.ContainsByPredicate([&State](const FBindingState& InState)
        { return InState._Key == State._Key && NOT InState._Unloading; });
        if (HasResidentIdentity)
        { continue; }

        _Bindings.Add(MoveTemp(State));
    }
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::DoHandle_LevelRemoved(ULevel* InLevel, UWorld* InWorld) -> void
{
    if (InWorld != GetWorld() || InLevel == nullptr)
    { return; }

    for (auto& Binding : _Bindings)
    {
        if (Binding._Level.Get() == InLevel)
        {
            // The actor can vanish immediately after this delegate. Preserve a tombstone carrying the
            // key, owner fence, and source state until the registry accepts the unload.
            Binding._AwaitingOwner = false;
            Binding._Unloading = true;
            DoUnload(Binding);
        }
    }
    _Bindings.RemoveAll([InLevel](const FBindingState& InState)
    { return InState._Level.Get() == InLevel && InState._Unloading && NOT InState._Loaded; });
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::DoTry_Load(FBindingState& InOutState) -> void
{
    auto* World = GetWorld();
    auto* Binding = InOutState._Binding.Get();
    if (World == nullptr || Binding == nullptr || NOT Binding->Get_HasValidIdentity() || NOT InOutState._Key.Get_IsValid())
    { InOutState._TerminalFailure = true; return; }

    const auto& Selector = InOutState._DataLayerSelector;
    if (NOT Selector.Get_IsCanonical())
    { InOutState._TerminalFailure = true; return; }

    const auto Owner = ck::groundnav::stream_partitions::TryGet_ManifestOwner(World, InOutState._Key._VolumeId);
    const auto Snapshot = ck::groundnav::world_fields::TryGet_StreamOwnerSnapshot(World, InOutState._Key._VolumeId);
    if (NOT Owner.IsSet() || NOT Owner->Get_IsValid() || NOT Snapshot.IsSet() || NOT Snapshot->_DefaultField.IsValid())
    { return; }
    if (Owner->_DataLayerSelector.Get_LayerNames() != Selector.Get_LayerNames())
    { InOutState._TerminalFailure = true; return; }

    auto Template = ck::groundnav::FCk_GroundNav_StreamFieldBundle{};
    Template._DefaultField = *Snapshot->_DefaultField;
    for (const auto& Variant : Snapshot->_VariantFields)
    {
        if (NOT Variant.Key.IsValid() || NOT Variant.Value.IsValid())
        { InOutState._TerminalFailure = true; return; }
        Template._VariantFields.Add(Variant.Key, *Variant.Value);
    }

    const auto DerivedManifestPath = ck::groundnav::Get_CookedSourceManifestAssetPath(
        Owner->_SourceLevelPackage, Owner->_CookKey, Owner->_DataLayerSelector, Binding->_PartitionId);
    if (DerivedManifestPath.IsEmpty() ||
        (NOT Binding->_Manifest.IsNull() && Binding->_Manifest.ToSoftObjectPath().ToString() != DerivedManifestPath))
    { InOutState._TerminalFailure = true; return; }

    auto* Manifest = LoadObject<UCk_GroundNav_CookedSourceManifest_UE>(
        nullptr, *DerivedManifestPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
    auto Transitions = TArray<ck::groundnav::FCk_GroundNav_StreamTileTransition>{};
    if (Manifest == nullptr || Try_LoadBoundCookedSourceManifest(
            *Manifest, InOutState._TileCoords, Template, Owner->_DefaultFingerprint,
            Owner->_VariantFingerprints, Binding->_StreamingVolumeId, Binding->_PartitionId,
            Selector, Transitions) != ECk_GroundNav_CookStatus::Cooked)
    { InOutState._TerminalFailure = true; return; }

    auto Load = ck::groundnav::stream_partitions::FCk_GroundNav_StreamPartitionLoad{};
    Load._Key = InOutState._Key;
    Load._OwnerInstance = Owner->_OwnerInstance;
    Load._Generation = Get_NextGeneration(InOutState._Key);
    Load._Transitions = MoveTemp(Transitions);
    const auto Result = ck::groundnav::stream_partitions::Load(World, Load);
    if (Result.Get_Succeeded())
    {
        InOutState._OwnerInstance = Load._OwnerInstance;
        InOutState._Generation = Load._Generation;
        InOutState._AwaitingOwner = false;
        InOutState._Loaded = true;
        InOutState._DataLayersActivated = true;
        return;
    }

    // A newly registered owner can replace a previous lifetime between the two snapshots; retry only
    // that ordinary ordering race. Asset/identity/duplicate refusals stay terminal and publish nothing.
    InOutState._TerminalFailure = Result._Status != ck::groundnav::stream_partitions::ECk_GroundNav_StreamPartitionLifecycleStatus::StaleEvent;
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::DoUnload(FBindingState& InOutState) -> void
{
    if (NOT InOutState._Loaded)
    { return; }
    if (GetWorld() == nullptr)
    { return; }

    const auto Generation = Get_NextGeneration(InOutState._Key);
    const auto Result = ck::groundnav::stream_partitions::Unload(
        GetWorld(), InOutState._Key, InOutState._OwnerInstance, Generation);
    if (Result.Get_Succeeded())
    {
        InOutState._Generation = Generation;
        InOutState._Loaded = false;
        InOutState._DataLayersActivated = false;
    }
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::DoUpdate_DataLayerActivation(FBindingState& InOutState) -> void
{
    const auto DesiredActivated = Get_AreDataLayersActivated(InOutState);
    if (DesiredActivated == InOutState._DataLayersActivated)
    { return; }

    const auto Generation = Get_NextGeneration(InOutState._Key);
    const auto Result = DesiredActivated
        ? ck::groundnav::stream_partitions::Reactivate(GetWorld(), InOutState._Key, InOutState._OwnerInstance, Generation)
        : ck::groundnav::stream_partitions::Deactivate(GetWorld(), InOutState._Key, InOutState._OwnerInstance, Generation);
    if (Result.Get_Succeeded())
    {
        InOutState._Generation = Generation;
        InOutState._DataLayersActivated = DesiredActivated;
    }
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::Get_AreDataLayersActivated(const FBindingState& InState) const -> bool
{
    // The empty canonical selector deliberately means collect-all and therefore has no runtime layer
    // gate. A nonempty selector is one cooked geometry population: every named layer must be activated.
    const auto& LayerNames = InState._DataLayerSelector.Get_LayerNames();
    if (LayerNames.IsEmpty())
    { return true; }

    const auto* World = GetWorld();
    const auto* DataLayerManager = UDataLayerManager::GetDataLayerManager(World);
    if (DataLayerManager == nullptr)
    { return false; }

    auto States = TArray<EDataLayerRuntimeState>{};
    States.Reserve(LayerNames.Num());
    for (const auto LayerName : LayerNames)
    {
        const auto* Instance = DataLayerManager->GetDataLayerInstanceFromName(LayerName);
        if (Instance == nullptr)
        { return false; }
        States.Emplace(DataLayerManager->GetDataLayerInstanceEffectiveRuntimeState(Instance));
    }
    return Get_AreDataLayerRuntimeStatesActivated(States);
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::Get_IsBlockedByUnloadingSource(
    const FBindingState& InState) const -> bool
{
    return _Bindings.ContainsByPredicate([&InState](const FBindingState& InOther)
    {
        return &InOther != &InState && InOther._Key == InState._Key &&
               InOther._Unloading && InOther._Loaded;
    });
}

auto UCk_GroundNav_StreamManifestSubsystem_UE::Get_NextGeneration(
    const ck::groundnav::stream_partitions::FCk_GroundNav_StreamPartitionKey& InKey) -> uint64
{
    auto& Generation = _GenerationLedger.FindOrAdd(InKey);
    ++Generation;
    return Generation;
}

// --------------------------------------------------------------------------------------------------------------------
