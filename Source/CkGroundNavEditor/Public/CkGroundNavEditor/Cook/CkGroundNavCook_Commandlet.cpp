#include "CkGroundNavEditor/Cook/CkGroundNavCook_Commandlet.h"

#include "CkGroundNavEditor/Cook/CkGroundNavCook_FieldCooker.h"
#include "CkGroundNavEditor/Cook/CkGroundNavCook_GeometrySnapshotBackend.h"
#include "../../../CkGroundNavEditor_Log.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEntitySpawner/CkEntitySpawner_Actor.h"
#include "CkGroundNav/Cook/CkGroundNav_CookedFieldIndex.h"
#include "CkGroundNav/Streaming/CkGroundNav_StreamManifestBinding_UE.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_EntityScript.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJoltEditor/Cook/CkJoltCook_MapSelection.h"
#include "CkJoltEditor/Cook/CkJoltCook_GeometrySnapshot.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <AssetRegistry/IAssetRegistry.h>
#include <FileHelpers.h>
#include <Engine/Level.h>
#include <Engine/LevelStreaming.h>
#include <EngineUtils.h>
#include <Misc/PackageName.h>
#include <Misc/Parse.h>
#include <UObject/Package.h>
#include <UObject/UObjectGlobals.h>
#include <WorldPartition/WorldPartition.h>
#include <WorldPartition/WorldPartitionActorDescInstance.h>
#include <WorldPartition/WorldPartitionHelpers.h>
#include <Settings/ProjectPackagingSettings.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_cook_commandlet
{
    struct FCookCandidate
    {
        FName _SpawnerName;
        FCk_Fragment_GroundNavVolume_ParamsData _Params;
        FName _SourceLevel;
        TArray<ck::groundnav::cook::FCk_GroundNav_CookFieldPlan> _Plans;
    };

    /** A value copy of a cell-resident manifest binding. Descriptor-owned actors are released after each callback. */
    struct FSourceManifestBindingCandidate
    {
        int32 _StreamingVolumeId = INDEX_NONE;
        int32 _PartitionId = INDEX_NONE;
        ck::groundnav::FCk_GroundNav_DataLayerSelector _DataLayerSelector;
        TArray<FIntPoint> _TileCoords;
        FString _AuthoredManifestPath;
    };

    static auto Discover_AlwaysCookMapCandidates(const TArray<FString>& InDirectories) -> TArray<FString>
    {
        auto Mounted = TArray<FString>{};
        for (auto Directory : InDirectories)
        {
            while (Directory.Len() > 1 && Directory.EndsWith(TEXT("/")))
            { Directory.LeftChopInline(1); }
            if (!Directory.StartsWith(TEXT("/")))
            { Directory = TEXT("/Game/") + Directory; }

            auto Filename = FString{};
            if (FPackageName::TryConvertLongPackageNameToFilename(Directory, Filename))
            { Mounted.AddUnique(Directory); }
        }

        if (Mounted.IsEmpty())
        { return {}; }

        auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
        Registry.ScanPathsSynchronous(Mounted);

        auto Filter = FARFilter{};
        Filter.ClassPaths.Add(UWorld::StaticClass()->GetClassPathName());
        Filter.bRecursivePaths = true;
        Filter.bIncludeOnlyOnDiskAssets = true;
        for (const auto& Directory : Mounted)
        { Filter.PackagePaths.Add(*Directory); }

        auto Assets = TArray<FAssetData>{};
        Registry.GetAssets(Filter, Assets);

        auto Result = TArray<FString>{};
        for (const auto& Asset : Assets)
        { Result.Add(Asset.PackageName.ToString()); }
        return Result;
    }

    static auto Discover_AllMapCandidates(const FString& InRoot) -> TArray<FString>
    {
        auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
        Registry.SearchAllAssets(true);

        auto Filter = FARFilter{};
        Filter.ClassPaths.Add(UWorld::StaticClass()->GetClassPathName());
        Filter.PackagePaths.Add(*InRoot);
        Filter.bRecursivePaths = true;
        Filter.bIncludeOnlyOnDiskAssets = true;

        auto Assets = TArray<FAssetData>{};
        Registry.GetAssets(Filter, Assets);
        auto Result = TArray<FString>{};
        for (const auto& Asset : Assets)
        { Result.Add(Asset.PackageName.ToString()); }
        return Result;
    }

    static auto Get_ResolvedLevel(const ULevelStreaming& InStreamingLevel) -> ULevel*
    {
        if (auto* Loaded = InStreamingLevel.GetLoadedLevel(); ck::IsValid(Loaded))
        { return Loaded; }

        auto* Package = FindPackage(nullptr, *InStreamingLevel.GetWorldAssetPackageName());
        auto* LevelWorld = Package == nullptr ? nullptr : UWorld::FindWorldInPackage(Package);
        return ck::IsValid(LevelWorld) ? LevelWorld->PersistentLevel : nullptr;
    }

    static auto Ensure_StreamingLevels(UWorld& InWorld) -> bool
    {
        for (auto* StreamingLevel : InWorld.GetStreamingLevels())
        {
            if (ck::Is_NOT_Valid(StreamingLevel))
            { continue; }

            const auto HasIdentityTransform = StreamingLevel->LevelTransform.Equals(FTransform::Identity);
            CK_ENSURE_IF_NOT(HasIdentityTransform,
                TEXT("GroundNavCook: streaming level [{}] has a non-identity instance transform; source-level "
                     "cook identity cannot distinguish its transformed placement, so refusing the cook"),
                StreamingLevel->GetWorldAssetPackageName())
            { }
            if (NOT HasIdentityTransform)
            { return false; }

            // LoadMap may leave an authored streaming level unloaded according to its runtime flags.
            // A cook must visit authored geometry, not only what happened to be visible at map open.
            auto* LoadedLevel = Get_ResolvedLevel(*StreamingLevel);
            if (ck::Is_NOT_Valid(LoadedLevel))
            {
                LoadPackage(nullptr, *StreamingLevel->GetWorldAssetPackageName(), LOAD_None);
                LoadedLevel = Get_ResolvedLevel(*StreamingLevel);
            }
            const auto LevelIsResolved = ck::IsValid(LoadedLevel);
            CK_ENSURE_IF_NOT(LevelIsResolved,
                TEXT("GroundNavCook: streaming level [{}] did not load; refusing a partial cook"),
                StreamingLevel->GetWorldAssetPackageName())
            { }
            if (NOT LevelIsResolved)
            { return false; }

            LoadedLevel->OwningWorld = &InWorld;
            if (!InWorld.GetLevels().Contains(LoadedLevel))
            {
                constexpr auto ConsiderTimeLimit = false;
                InWorld.AddToWorld(LoadedLevel, StreamingLevel->LevelTransform, ConsiderTimeLimit);
            }

            const auto LevelIsInWorld = InWorld.GetLevels().Contains(LoadedLevel);
            CK_ENSURE_IF_NOT(LevelIsInWorld,
                TEXT("GroundNavCook: streaming level [{}] could not join the map world; refusing a partial cook"),
                StreamingLevel->GetWorldAssetPackageName())
            { }
            if (NOT LevelIsInWorld)
            { return false; }
        }

        return true;
    }

    /**
     * Copies the authoring state needed after a World Partition descriptor walk releases its actor.
     * A plan deliberately never retains an actor, level or descriptor pointer: the cooker consumes
     * values after the walk and may therefore save only the exact preflighted values it discovered.
     */
    static auto
        Try_AddCookCandidate(
            const AActor&                  InActor,
            TArray<FCookCandidate>&        OutCandidates,
            TArray<ck::groundnav::cook::FCk_GroundNav_CookIdentity>& OutIdentities)
        -> bool
    {
        const auto* Spawner = Cast<ACk_EntitySpawner_UE>(&InActor);
        if (Spawner == nullptr)
        { return true; }

        const auto* Script = Cast<UCk_GroundNavVolume_EntityScript>(Spawner->Get_EntityScript());
        if (Script == nullptr)
        { return true; }

        auto PlacedParams = ck::groundnav::Get_PlacedVolumeParams(
            Script->Get_Params(), Spawner->GetActorTransform());
        if (PlacedParams.Get_CookKey().IsNone())
        { return true; }

        const auto* Level = Spawner->GetLevel();
        const auto LevelIsValid = ck::IsValid(Level) && ck::IsValid(Level->GetOutermost());
        CK_ENSURE_IF_NOT(LevelIsValid, TEXT("GroundNavCook: spawner [{}] has no source level"),
            Spawner->GetName())
        { }
        if (NOT LevelIsValid)
        { return false; }

        const auto SourceLevel = ck::groundnav::Get_PackageLookupKey(Level->GetOutermost()->GetName());
        // Keep the params consumed by runtime owner registration on the same canonical source-level
        // identity used to name this commandlet's generated field and manifest assets.
        PlacedParams.Set_CookLevelPackage(SourceLevel);
        OutIdentities.Emplace(ck::groundnav::cook::FCk_GroundNav_CookIdentity{
            SourceLevel, PlacedParams.Get_CookKey(), PlacedParams.Get_StreamingVolumeId(),
            PlacedParams.Get_DataLayerSelector()});

        auto Candidate = FCookCandidate{};
        Candidate._SpawnerName = Spawner->GetFName();
        Candidate._Params = MoveTemp(PlacedParams);
        Candidate._SourceLevel = SourceLevel;
        OutCandidates.Emplace(MoveTemp(Candidate));
        return true;
    }

    static auto
        Try_AddSourceManifestBinding(
            const AActor& InActor,
            TArray<FSourceManifestBindingCandidate>& OutBindings)
        -> bool
    {
        const auto* Binding = Cast<ACk_GroundNav_StreamManifestBinding_UE>(&InActor);
        if (Binding == nullptr)
        { return true; }

        auto Selector = ck::groundnav::FCk_GroundNav_DataLayerSelector{};
        const auto BindingIsValid = Binding->Get_HasValidIdentity() &&
            Binding->TryGet_DataLayerSelector(Selector);
        CK_ENSURE_IF_NOT(BindingIsValid,
            TEXT("GroundNavCook: stream manifest binding [{}] needs positive ids, canonical selector, and sorted tile coordinates"),
            Binding->GetName())
        { }
        if (NOT BindingIsValid)
        { return false; }

        auto Candidate = FSourceManifestBindingCandidate{};
        Candidate._StreamingVolumeId = Binding->_StreamingVolumeId;
        Candidate._PartitionId = Binding->_PartitionId;
        Candidate._DataLayerSelector = MoveTemp(Selector);
        Candidate._TileCoords = Binding->_TileCoords;
        Candidate._AuthoredManifestPath = Binding->_Manifest.IsNull()
            ? FString{}
            : Binding->_Manifest.ToSoftObjectPath().ToString();
        OutBindings.Emplace(MoveTemp(Candidate));
        return true;
    }

    static auto Get_IsManifestDriven(const FCookCandidate& InCandidate) -> bool
    {
        return InCandidate._Params.Get_StreamingBuildScope() == ECk_GroundNav_StreamingBuildScope::ManifestDriven;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GroundNavCook_Commandlet::
    Main(
        const FString& InParams)
    -> int32
{
    auto Tokens = TArray<FString>{};
    auto Switches = TArray<FString>{};
    auto Params = TMap<FString, FString>{};
    ParseCommandLine(*InParams, Tokens, Switches, Params);

    const auto IsDryRun = Switches.Contains(TEXT("DryRun"));
    const auto HasMap = Params.Contains(TEXT("Map"));
    const auto CookAllMaps = Switches.Contains(TEXT("AllMaps"));
    const auto CookPackagingMaps = Switches.Contains(TEXT("PackagingMaps"));
    const auto HasOneSelectionMode = static_cast<int32>(HasMap) + static_cast<int32>(CookAllMaps) +
        static_cast<int32>(CookPackagingMaps) == 1;
    CK_ENSURE_IF_NOT(HasOneSelectionMode,
        TEXT("GroundNavCook: pass exactly one of -Map, -AllMaps, or -PackagingMaps"))
    { }
    if (NOT HasOneSelectionMode)
    { return 1; }

    if (CookAllMaps || CookPackagingMaps)
    {
        auto Maps = TArray<FString>{};

        if (CookAllMaps)
        {
            const auto Root = Params.Contains(TEXT("Root")) ? Params[TEXT("Root")] : FString{TEXT("/Game")};
            const auto RootIsValid = FPackageName::IsValidLongPackageName(Root);
            CK_ENSURE_IF_NOT(RootIsValid, TEXT("GroundNavCook: -Root [{}] is not a valid package path"), Root)
            { }
            if (NOT RootIsValid)
            { return 1; }

            Maps = ck_groundnav_cook_commandlet::Discover_AllMapCandidates(Root);
            const auto GeneratedDataPath = TArray<FString>{ck::groundnav::kCookedDataRootPath};
            Maps.RemoveAll([&](const FString& InMap)
                { return ck::jolt::cook::Get_IsPackageExcluded(InMap, GeneratedDataPath); });
        }
        else
        {
            const auto* Settings = GetDefault<UProjectPackagingSettings>();
            const auto SettingsAreValid = ck::IsValid(Settings);
            CK_ENSURE_IF_NOT(SettingsAreValid, TEXT("GroundNavCook: packaging settings are unavailable"))
            { }
            if (NOT SettingsAreValid)
            { return 1; }

            auto Input = ck::jolt::cook::FCk_Jolt_PackagingMapSelectionInput{};
            Input._bPackagingMaps = true;
            Input._bCookAll = Settings->bCookAll;
            Input._CookedDataRootPath = ck::groundnav::kCookedDataRootPath;
            for (const auto& Entry : Settings->MapsToCook) { Input._AuthoredMapsToCook.Add(Entry.FilePath); }
            for (const auto& Directory : Settings->DirectoriesToAlwaysCook) { Input._DirectoriesToAlwaysCook.Add(Directory.Path); }
            for (const auto& Directory : Settings->DirectoriesToNeverCook) { Input._DirectoriesToNeverCook.Add(Directory.Path); }
            Input._DiscoveredAlwaysCookMapCandidates =
                ck_groundnav_cook_commandlet::Discover_AlwaysCookMapCandidates(Input._DirectoriesToAlwaysCook);

            const auto Selection = ck::jolt::cook::Select_PackagingMaps(Input);
            CK_ENSURE_IF_NOT(Selection._Success, TEXT("GroundNavCook: -PackagingMaps rejected: [{}]"), Selection._Failure)
            { }
            if (NOT Selection._Success)
            { return 1; }
            Maps = Selection._MapPackageNames;
        }

        const auto HasMaps = !Maps.IsEmpty();
        CK_ENSURE_IF_NOT(HasMaps, TEXT("GroundNavCook: selected no maps"))
        { }
        if (NOT HasMaps)
        { return 1; }

        auto FailureCount = 0;
        for (const auto& Map : Maps)
        {
            const auto ChildParams = FString::Printf(TEXT("-Map=%s%s"), *Map, IsDryRun ? TEXT(" -DryRun") : TEXT(""));
            if (Main(ChildParams) != 0)
            { ++FailureCount; }
        }
        return FailureCount == 0 ? 0 : 1;
    }

    const auto MapPackage = Params[TEXT("Map")];

    const auto SelectionIsValid = HasMap && FPackageName::IsValidLongPackageName(MapPackage);
    CK_ENSURE_IF_NOT(SelectionIsValid,
        TEXT("GroundNavCook: exactly one -Map=/Game/... is required"))
    { }
    if (NOT SelectionIsValid)
    { return 1; }

    // Editor-world subsystem eligibility is read while LoadMap builds the world's subsystem collection.
    // The scoped override is in-memory only and restores before the commandlet returns.
    auto StaticWorldMode = FCk_Jolt_ScopedEditorStaticWorldModeOverride{
        ECk_Jolt_EditorStaticWorldMode::LiveExtract};
    const auto OverrideApplied = StaticWorldMode.Get_IsApplied();
    CK_ENSURE_IF_NOT(OverrideApplied, TEXT("GroundNavCook: could not enable editor Jolt extraction"))
    { }
    if (NOT OverrideApplied)
    { return 1; }

    const auto MapFilename = FPackageName::LongPackageNameToFilename(
        MapPackage, FPackageName::GetMapPackageExtension());
    auto* World = UEditorLoadingAndSavingUtils::LoadMap(MapFilename);

    const auto WorldIsValid = ck::IsValid(World);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("GroundNavCook: failed to load map [{}]"), MapPackage)
    { }
    if (NOT WorldIsValid)
    { return 1; }

    if (NOT ck_groundnav_cook_commandlet::Ensure_StreamingLevels(*World))
    { return 1; }

    if (NOT ck::groundnav::cook::FCk_GroundNav_FieldCooker::Prepare_WorldGeometry(*World))
    { return 1; }

    // World Partition releases each actor after its callback. Capture the production LevelSweep geometry
    // now as values, so every later plan sees the same descriptor-complete world without retaining UObjects.
    const auto UsesWorldPartitionSnapshot = World->GetWorldPartition() != nullptr;
    auto GeometrySnapshot = ck::jolt::cook::FCk_Jolt_CookGeometrySnapshot{};

    auto Candidates = TArray<ck_groundnav_cook_commandlet::FCookCandidate>{};
    auto Identities = TArray<ck::groundnav::cook::FCk_GroundNav_CookIdentity>{};
    auto SourceManifestBindings = TArray<ck_groundnav_cook_commandlet::FSourceManifestBindingCandidate>{};

    // Seed the identity set before the descriptor walk. World Partition may return an actor that is
    // already present in a loaded cell; baking it twice would silently create duplicate cook identity
    // diagnostics that depend on the map's initial streaming state.
    auto VisitedActorGuids = TSet<FGuid>{};
    auto VisitedActorPaths = TSet<FString>{};
    for (TActorIterator<AActor> It{World}; It; ++It)
    {
        if (It->GetActorGuid().IsValid())
        { VisitedActorGuids.Add(It->GetActorGuid()); }
        VisitedActorPaths.Add(It->GetPathName());
        if (UsesWorldPartitionSnapshot && NOT GeometrySnapshot.Try_AddActor(**It))
        { return 1; }
    }

    for (TActorIterator<ACk_EntitySpawner_UE> It{World}; It; ++It)
    {
        if (NOT ck_groundnav_cook_commandlet::Try_AddCookCandidate(**It, Candidates, Identities))
        { return 1; }
    }
    for (TActorIterator<ACk_GroundNav_StreamManifestBinding_UE> It{World}; It; ++It)
    {
        if (NOT ck_groundnav_cook_commandlet::Try_AddSourceManifestBinding(**It, SourceManifestBindings))
        { return 1; }
    }

    // The helper loads descriptor actors in bounded batches and releases them after each callback.
    // Copy just the authoring values above while the actor exists; retaining its UObject would defeat
    // the batch lifetime and retaining a descriptor would cross the editor/runtime boundary.
    if (auto* WorldPartition = World->GetWorldPartition())
    {
        auto DescriptorWalkSucceeded = true;
        FWorldPartitionHelpers::ForEachActorWithLoading(WorldPartition,
            [&](const FWorldPartitionActorDescInstance* InActorDescInstance) -> bool
            {
                if (InActorDescInstance == nullptr)
                { return true; }

                const auto ActorGuid = InActorDescInstance->GetGuid();
                if (ActorGuid.IsValid() && VisitedActorGuids.Contains(ActorGuid))
                { return true; }

                const auto* Actor = InActorDescInstance->GetActor();
                if (ck::Is_NOT_Valid(Actor))
                { return true; }

                const auto ActorPath = Actor->GetPathName();
                if (VisitedActorPaths.Contains(ActorPath))
                { return true; }
                if (ActorGuid.IsValid())
                { VisitedActorGuids.Add(ActorGuid); }
                VisitedActorPaths.Add(ActorPath);
                if (NOT GeometrySnapshot.Try_AddActor(*Actor))
                { DescriptorWalkSucceeded = false; return false; }

                DescriptorWalkSucceeded = ck_groundnav_cook_commandlet::Try_AddCookCandidate(
                    *Actor, Candidates, Identities) &&
                    ck_groundnav_cook_commandlet::Try_AddSourceManifestBinding(*Actor, SourceManifestBindings);
                return DescriptorWalkSucceeded;
            });

        const auto DescriptorTraversalCompleted = DescriptorWalkSucceeded;
        CK_ENSURE_IF_NOT(DescriptorTraversalCompleted,
            TEXT("GroundNavCook: World Partition descriptor traversal could not read every GroundNav volume"))
        { }
        if (NOT DescriptorTraversalCompleted)
        { return 1; }
    }

    const auto CookIdentitiesAreUnique =
        ck::groundnav::cook::FCk_GroundNav_FieldCooker::Get_AreCookIdentitiesUnique(Identities);
    CK_ENSURE_IF_NOT(CookIdentitiesAreUnique,
        TEXT("GroundNavCook: duplicate or incomplete {source level, cook key} identity; no assets were written"))
    { }
    if (NOT CookIdentitiesAreUnique)
    { return 1; }

    for (auto& Candidate : Candidates)
    {
        auto Stats = ck::groundnav::cook::FCk_GroundNav_CookFieldStats{};
        if (UsesWorldPartitionSnapshot)
        {
            auto Selector = ck::groundnav::FCk_GroundNav_DataLayerSelector{};
            if (NOT ck::groundnav::TryMake_DataLayerSelector(Candidate._Params.Get_DataLayerSelector().Get_LayerNames(), Selector))
            { return 1; }
            auto GeometryBackend = ck::groundnav::cook::FCk_GroundNav_CookGeometrySnapshotBackend{GeometrySnapshot, MoveTemp(Selector)};
            Stats = ck::groundnav::cook::FCk_GroundNav_FieldCooker::Cook_Volume(*World, Candidate._Params,
                Candidate._SourceLevel, ck::groundnav::cook::ECk_GroundNav_CookMode::DryRun, Candidate._Plans, &GeometryBackend);
        }
        else
        {
            Stats = ck::groundnav::cook::FCk_GroundNav_FieldCooker::Cook_Volume(*World, Candidate._Params,
                Candidate._SourceLevel, ck::groundnav::cook::ECk_GroundNav_CookMode::DryRun, Candidate._Plans);
        }

        if (NOT Stats._Success)
        { return 1; }

        ck::groundnav_editor::Log(TEXT("GroundNavCook: [{}] planned {} field(s), {} tile(s)"),
            Candidate._SpawnerName, Stats._NumFields, Stats._NumTiles);
    }

    // Bindings are authored source ownership, never inferred from a World Partition cell or descriptor.
    // Validate every relation after every pure bake succeeds, before any generated asset package is created.
    auto SourceManifestPlans = TArray<ck::groundnav::cook::FCk_GroundNav_CookSourceManifestPlan>{};
    auto SeenSourcePartitionKeys = TSet<FString>{};
    for (const auto& Binding : SourceManifestBindings)
    {
        const auto PartitionKey = FString::Printf(TEXT("%d:%d"), Binding._StreamingVolumeId, Binding._PartitionId);
        const auto PartitionIsUnique = !SeenSourcePartitionKeys.Contains(PartitionKey);
        CK_ENSURE_IF_NOT(PartitionIsUnique,
            TEXT("GroundNavCook: duplicate stream manifest binding for {volume {}, partition {}}"),
            Binding._StreamingVolumeId, Binding._PartitionId)
        { }
        if (NOT PartitionIsUnique)
        { return 1; }
        SeenSourcePartitionKeys.Add(PartitionKey);

        auto MatchingCandidateIndices = TArray<int32>{};
        for (auto CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
        {
            const auto& Candidate = Candidates[CandidateIndex];
            if (ck_groundnav_cook_commandlet::Get_IsManifestDriven(Candidate) &&
                Candidate._Params.Get_StreamingVolumeId() == Binding._StreamingVolumeId)
            { MatchingCandidateIndices.Add(CandidateIndex); }
        }

        const auto HasOneManifestDrivenOwner = MatchingCandidateIndices.Num() == 1;
        CK_ENSURE_IF_NOT(HasOneManifestDrivenOwner,
            TEXT("GroundNavCook: stream manifest binding {volume {}, partition {}} must match exactly one ManifestDriven volume"),
            Binding._StreamingVolumeId, Binding._PartitionId)
        { }
        if (NOT HasOneManifestDrivenOwner)
        { return 1; }

        const auto& Owner = Candidates[MatchingCandidateIndices[0]];
        const auto SelectorMatchesOwner = Owner._Params.Get_DataLayerSelector().Get_LayerNames() ==
            Binding._DataLayerSelector.Get_LayerNames();
        CK_ENSURE_IF_NOT(SelectorMatchesOwner,
            TEXT("GroundNavCook: stream manifest binding {volume {}, partition {}} selector differs from its ManifestDriven volume"),
            Binding._StreamingVolumeId, Binding._PartitionId)
        { }
        if (NOT SelectorMatchesOwner)
        { return 1; }
    }

    for (auto CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
    {
        const auto& Candidate = Candidates[CandidateIndex];
        if (NOT ck_groundnav_cook_commandlet::Get_IsManifestDriven(Candidate))
        { continue; }

        auto Partitions = TArray<ck::groundnav::cook::FCk_GroundNav_CookSourcePartition>{};
        for (const auto& Binding : SourceManifestBindings)
        {
            if (Binding._StreamingVolumeId != Candidate._Params.Get_StreamingVolumeId())
            { continue; }

            auto Partition = ck::groundnav::cook::FCk_GroundNav_CookSourcePartition{};
            Partition._StreamingVolumeId = Binding._StreamingVolumeId;
            Partition._PartitionId = Binding._PartitionId;
            Partition._DataLayerSelector = Binding._DataLayerSelector;
            Partition._TileCoords = Binding._TileCoords;
            Partitions.Emplace(MoveTemp(Partition));
        }

        const auto HasBindings = !Partitions.IsEmpty();
        CK_ENSURE_IF_NOT(HasBindings,
            TEXT("GroundNavCook: ManifestDriven volume [{}] has no explicit stream manifest bindings"),
            Candidate._SpawnerName)
        { }
        if (NOT HasBindings)
        { return 1; }

        auto CandidateManifestPlans = TArray<ck::groundnav::cook::FCk_GroundNav_CookSourceManifestPlan>{};
        if (NOT ck::groundnav::cook::FCk_GroundNav_FieldCooker::Try_BuildSourceManifestPlans(
            Candidate._Plans, Partitions, CandidateManifestPlans))
        { return 1; }

        for (const auto& ManifestPlan : CandidateManifestPlans)
        {
            const auto* Binding = SourceManifestBindings.FindByPredicate([&ManifestPlan](const auto& InBinding)
            {
                return InBinding._StreamingVolumeId == ManifestPlan._StreamingVolumeId &&
                    InBinding._PartitionId == ManifestPlan._PartitionId;
            });
            const auto AssetPathMatchesBinding = Binding != nullptr &&
                (Binding->_AuthoredManifestPath.IsEmpty() || Binding->_AuthoredManifestPath == ManifestPlan._AssetPath);
            CK_ENSURE_IF_NOT(AssetPathMatchesBinding,
                TEXT("GroundNavCook: binding for {volume {}, partition {}} names a different generated manifest asset"),
                ManifestPlan._StreamingVolumeId, ManifestPlan._PartitionId)
            { }
            if (NOT AssetPathMatchesBinding)
            { return 1; }
        }

        SourceManifestPlans.Append(MoveTemp(CandidateManifestPlans));
    }

    if (IsDryRun)
    { return 0; }

    // All authored identities and pure bakes held before any package is created. An unsupported volume
    // therefore rejects the map with zero generated output rather than after another volume was saved.
    auto FieldPlans = TArray<ck::groundnav::cook::FCk_GroundNav_CookFieldPlan>{};
    for (const auto& Candidate : Candidates)
    { FieldPlans.Append(Candidate._Plans); }
    const auto Stats = ck::groundnav::cook::FCk_GroundNav_FieldCooker::Save_Plans(FieldPlans);
    if (NOT Stats._Success)
    { return 1; }

    if (!SourceManifestPlans.IsEmpty() &&
        NOT ck::groundnav::cook::FCk_GroundNav_FieldCooker::Save_SourceManifestPlans(SourceManifestPlans))
    { return 1; }

    ck::groundnav_editor::Log(TEXT("GroundNavCook: wrote {} field asset(s) and {} source manifest(s)"),
        Stats._NumAssetsWritten, SourceManifestPlans.Num());

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
