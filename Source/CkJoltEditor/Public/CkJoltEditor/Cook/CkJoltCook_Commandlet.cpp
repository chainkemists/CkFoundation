#include "CkJoltCook_Commandlet.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJoltEditor/Cook/CkJoltCook_MeshShapeCooker.h"
#include "CkJoltEditor/Cook/CkJoltCook_MapSelection.h"
#include "CkJoltEditor/Cook/CkJoltCook_WorldCooker.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <AssetRegistry/IAssetRegistry.h>
#include <Engine/Level.h>
#include <Engine/LevelStreaming.h>
#include <Engine/World.h>
#include <FileHelpers.h>
#include <HAL/FileManager.h>
#include <Misc/PackageName.h>
#include <Settings/ProjectPackagingSettings.h>
#include <UObject/Package.h>
#include <UObject/WeakObjectPtr.h>
#include <WorldPartition/WorldPartition.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_cook_commandlet
{
    static auto NormalizePackageDirectory(
        const FString& InPath) -> FString
    {
        if (InPath.IsEmpty())
        { return {}; }

        auto NormalizedPath = InPath;

        while (NormalizedPath.Len() > 1 && NormalizedPath.EndsWith(TEXT("/")))
        { NormalizedPath.LeftChopInline(1); }

        return NormalizedPath.StartsWith(TEXT("/"))
            ? NormalizedPath
            : TEXT("/Game/") + NormalizedPath;
    }

    /// Scans only configured, mounted AlwaysCook directories and reads UWorld asset metadata only.
    /// An unmounted non-content directory is not an error and never broadens the scan scope.
    static auto Discover_AlwaysCookMapCandidates(
        const TArray<FString>& InAlwaysCookDirectories) -> TArray<FString>
    {
        auto MountedDirectories = TArray<FString>{};

        for (const auto& Directory : InAlwaysCookDirectories)
        {
            const auto PackageDirectory = NormalizePackageDirectory(Directory);
            auto LocalDirectory = FString{};

            if (PackageDirectory.IsEmpty()
                || NOT FPackageName::TryConvertLongPackageNameToFilename(PackageDirectory, LocalDirectory))
            { continue; }

            MountedDirectories.AddUnique(PackageDirectory);
        }

        if (MountedDirectories.IsEmpty())
        { return {}; }

        auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
        AssetRegistry.ScanPathsSynchronous(MountedDirectories);

        auto Filter = FARFilter{};
        Filter.ClassPaths.Add(UWorld::StaticClass()->GetClassPathName());
        Filter.bRecursivePaths = true;
        Filter.bIncludeOnlyOnDiskAssets = true;

        for (const auto& Directory : MountedDirectories)
        { Filter.PackagePaths.Add(*Directory); }

        auto Assets = TArray<FAssetData>{};
        AssetRegistry.GetAssets(Filter, Assets);

        auto Candidates = TArray<FString>{};
        Candidates.Reserve(Assets.Num());

        for (const auto& Asset : Assets)
        { Candidates.Add(Asset.PackageName.ToString()); }

        return Candidates;
    }

    // The cooked-data root must be force-cooked into packaged builds — nothing hard-references
    // the assets (index is found by path convention), so a missing entry ships builds with NO
    // Jolt static world. Loud, with the exact line to add.
    /// The ULevel a streaming level refers to. GetLoadedLevel() is unset for a sublevel whose package
    /// was already in memory, but the ULevel is perfectly good — so resolve it from the package rather
    /// than treating it as absent. Both the load pass and the post-condition MUST agree on what
    /// "resolved" means, or the fallback silently fails the check it just satisfied.
    static auto Get_ResolvedLevel(const ULevelStreaming& InStreamingLevel) -> ULevel*
    {
        if (auto* Loaded = InStreamingLevel.GetLoadedLevel(); ck::IsValid(Loaded))
        { return Loaded; }

        auto* LevelPackage = FindPackage(nullptr, *InStreamingLevel.GetWorldAssetPackageName());
        if (LevelPackage == nullptr)
        { return nullptr; }

        auto* LevelWorld = UWorld::FindWorldInPackage(LevelPackage);
        if (ck::Is_NOT_Valid(LevelWorld))
        { return nullptr; }

        return LevelWorld->PersistentLevel;
    }

    static auto DoEnsure_AlwaysCookEntry() -> void
    {
        const auto RootPath = UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath();

        auto AlwaysCookDirectories = TArray<FString>{};
        GConfig->GetArray(TEXT("/Script/UnrealEd.ProjectPackagingSettings"),
            TEXT("DirectoriesToAlwaysCook"), AlwaysCookDirectories, GGameIni);

        const auto HasEntry = AlwaysCookDirectories.ContainsByPredicate(
            [&](const FString& InEntry)
            {
                return InEntry.Contains(RootPath);
            });

        CK_ENSURE_IF_NOT(HasEntry,
            TEXT("[{}] is NOT in DirectoriesToAlwaysCook — packaged builds will ship WITHOUT cooked Jolt data. "
                 "Add to DefaultGame.ini under [/Script/UnrealEd.ProjectPackagingSettings]:\n"
                 "+DirectoriesToAlwaysCook=(Path=\"{}\")"), RootPath, RootPath)
        { return; }
    }

    /// Map selection is intentionally world-free, but packaging entry roots must still resolve to
    /// real .umap files before any selected world is loaded.
    static auto DoEnsure_PackagingMapsExist(
        const TArray<FString>& InMapPackageNames) -> bool
    {
        for (const auto& MapPackageName : InMapPackageNames)
        {
            auto MapFilename = FString{};
            const auto HasMappedFilename = FPackageName::TryConvertLongPackageNameToFilename(
                MapPackageName, MapFilename, FPackageName::GetMapPackageExtension());
            const auto HasMapFile = HasMappedFilename && IFileManager::Get().FileExists(*MapFilename);

            CK_ENSURE_IF_NOT(HasMapFile,
                TEXT("CkJoltCook: -PackagingMaps entry [{}] does not resolve to a .umap file [{}]"),
                MapPackageName, MapFilename)
            { }

            if (NOT HasMapFile)
            { return false; }
        }

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltCook_Commandlet::
    Main(
        const FString& InParams)
    -> int32
{
    auto Tokens = TArray<FString>{};
    auto Switches = TArray<FString>{};
    auto ParamsMap = TMap<FString, FString>{};
    ParseCommandLine(*InParams, Tokens, Switches, ParamsMap);

    const auto CookMode = Switches.Contains(TEXT("DryRun"))
        ? ck::jolt::cook::ECk_Jolt_CookMode::DryRun
        : ck::jolt::cook::ECk_Jolt_CookMode::Cook;

    const auto CookPackagingMaps = Switches.Contains(TEXT("PackagingMaps"));
    const auto HasExplicitMap = ParamsMap.Contains(TEXT("Map"));
    const auto CookAllMaps = Switches.Contains(TEXT("AllMaps"));
    const auto Incremental = Switches.Contains(TEXT("Incremental"));
    const auto ForceRebuild = Switches.Contains(TEXT("ForceRebuild"));
    const auto RebuildModeIsValid = NOT (Incremental && ForceRebuild);
    // Routine freshness checks answer only the authored package entry worlds. A full rebake expands
    // into every AlwaysCook UWorld so new/forgotten maps are discovered by the explicit maintenance gate.
    const auto IncludeAlwaysCookDirectories = NOT Incremental;

    CK_ENSURE_IF_NOT(RebuildModeIsValid,
        TEXT("CkJoltCook: -Incremental and -ForceRebuild cannot be combined"))
    {}

    if (NOT RebuildModeIsValid)
    { return 1; }

    // Per-mesh shape sweep (-MeshShapes): independent of any map. May be combined with -Map,
    // -AllMaps, or -PackagingMaps, or run alone.
    const auto CookMeshShapes = Switches.Contains(TEXT("MeshShapes"));
    auto MeshShapesFailed = false;

    if (CookMeshShapes)
    {
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().SearchAllAssets(true);
        UE_LOG(CkJolt, Display, TEXT("CkJoltCook: mesh shapes (%s)"),
            ForceRebuild ? TEXT("full rebuild") : TEXT("freshness check"));
        const auto MeshStats = FCk_Jolt_MeshShapeCooker::Cook_MeshShapes(CookMode, ForceRebuild);
        MeshShapesFailed = NOT MeshStats._Success;
        UE_LOG(CkJolt, Display, TEXT("CkJoltCook: mesh shapes complete: %d rebuilt, %d current, %d failed"),
            MeshStats._NumShapesCooked, MeshStats._NumUpToDate, MeshStats._NumFailed);
    }

    CK_ENSURE_IF_NOT(NOT MeshShapesFailed,
        TEXT("CkJoltCook: mesh-shape sweep failed — refusing map selection or world loading"))
    { }

    if (MeshShapesFailed)
    { return 1; }

    auto PackagingMaps = ck::jolt::cook::FCk_Jolt_PackagingMapSelectionResult{};
    auto PackagingExcludedLevelPackagePaths = TArray<FString>{};

    if (CookPackagingMaps)
    {
        const auto PackagingSettings = TWeakObjectPtr<const UProjectPackagingSettings>{
            GetDefault<UProjectPackagingSettings>()};
        const auto HasPackagingSettings = PackagingSettings.IsValid();

        CK_ENSURE_IF_NOT(HasPackagingSettings,
            TEXT("CkJoltCook: UProjectPackagingSettings is unavailable — refusing -PackagingMaps"))
        { }

        if (NOT HasPackagingSettings)
        { return 1; }

        auto SelectionInput = ck::jolt::cook::FCk_Jolt_PackagingMapSelectionInput{};
        SelectionInput._bPackagingMaps = true;
        SelectionInput._bMap = HasExplicitMap;
        SelectionInput._bAllMaps = CookAllMaps;
        SelectionInput._bCookAll = PackagingSettings->bCookAll;
        SelectionInput._IncludeAlwaysCookDirectories = IncludeAlwaysCookDirectories;
        SelectionInput._CookedDataRootPath = UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath();

        for (const auto& Map : PackagingSettings->MapsToCook)
        { SelectionInput._AuthoredMapsToCook.Add(Map.FilePath); }
        for (const auto& Directory : PackagingSettings->DirectoriesToAlwaysCook)
        { SelectionInput._DirectoriesToAlwaysCook.Add(Directory.Path); }
        for (const auto& Directory : PackagingSettings->DirectoriesToNeverCook)
        {
            SelectionInput._DirectoriesToNeverCook.Add(Directory.Path);
            PackagingExcludedLevelPackagePaths.AddUnique(Directory.Path);
        }

        SelectionInput._JoltExcludedMapPathPrefixes = UCk_Utils_Jolt_ProjectSettings::Get_CookExcludedMapPathPrefixes();
        for (const auto& ExcludedPrefix : SelectionInput._JoltExcludedMapPathPrefixes)
        { PackagingExcludedLevelPackagePaths.AddUnique(ExcludedPrefix); }
        PackagingExcludedLevelPackagePaths.AddUnique(SelectionInput._CookedDataRootPath);
        if (SelectionInput._IncludeAlwaysCookDirectories)
        {
            SelectionInput._DiscoveredAlwaysCookMapCandidates =
                ck_jolt_cook_commandlet::Discover_AlwaysCookMapCandidates(SelectionInput._DirectoriesToAlwaysCook);
        }
        PackagingMaps = ck::jolt::cook::Select_PackagingMaps(SelectionInput);

        const auto HasValidPackagingMaps = PackagingMaps._Success;

        CK_ENSURE_IF_NOT(HasValidPackagingMaps,
            TEXT("CkJoltCook: -PackagingMaps selection rejected: [{}]"), PackagingMaps._Failure)
        { }

        if (NOT HasValidPackagingMaps)
        { return 1; }

        UE_LOG(CkJolt, Display, TEXT("CkJoltCook: packaging map policy [%s]: %d authored + %d AlwaysCook = %d selected"),
            SelectionInput._IncludeAlwaysCookDirectories ? TEXT("full union") : TEXT("incremental entry worlds"),
            PackagingMaps._NumAuthoredMaps, PackagingMaps._NumAlwaysCookMaps, PackagingMaps._MapPackageNames.Num());

        const auto HasSelectedPackagingMaps = NOT PackagingMaps._MapPackageNames.IsEmpty();

        CK_ENSURE_IF_NOT(HasSelectedPackagingMaps,
            TEXT("CkJoltCook: -PackagingMaps found no eligible UWorld entry maps in MapsToCook or DirectoriesToAlwaysCook"))
        { }

        if (NOT HasSelectedPackagingMaps)
        { return 1; }

        const auto PackagingMapsExist = ck_jolt_cook_commandlet::DoEnsure_PackagingMapsExist(
            PackagingMaps._MapPackageNames);

        if (NOT PackagingMapsExist)
        { return 1; }
    }

    ck_jolt_cook_commandlet::DoEnsure_AlwaysCookEntry();

    auto MapsToCook = TArray<FString>{};

    if (CookPackagingMaps)
    { MapsToCook = MoveTemp(PackagingMaps._MapPackageNames); }
    else if (const auto* SingleMap = ParamsMap.Find(TEXT("Map")))
    { MapsToCook.Add(*SingleMap); }
    else if (CookAllMaps)
    {
        const auto Root = ParamsMap.Contains(TEXT("Root")) ? ParamsMap[TEXT("Root")] : FString{TEXT("/Game")};
        const auto ExcludedPrefixes = UCk_Utils_Jolt_ProjectSettings::Get_CookExcludedMapPathPrefixes();

        auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
        AssetRegistry.SearchAllAssets(true);

        auto Filter = FARFilter{};
        Filter.ClassPaths.Add(UWorld::StaticClass()->GetClassPathName());
        Filter.PackagePaths.Add(*Root);
        Filter.bRecursivePaths = true;

        auto Assets = TArray<FAssetData>{};
        AssetRegistry.GetAssets(Filter, Assets);

        for (const auto& Asset : Assets)
        {
            const auto PackageName = Asset.PackageName.ToString();

            const auto IsExcluded = ExcludedPrefixes.ContainsByPredicate(
                [&](const FString& InPrefix)
                {
                    return PackageName.StartsWith(InPrefix);
                });

            if (IsExcluded)
            { continue; }

            // Never cook INTO the cooked-data root itself.
            if (PackageName.StartsWith(UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath()))
            { continue; }

            MapsToCook.Add(PackageName);
        }
    }

    const auto NothingRequested = MapsToCook.IsEmpty() && NOT CookMeshShapes;
    CK_ENSURE_IF_NOT(NOT NothingRequested,
        TEXT("CkJoltCook: nothing to cook — pass -Map=/Game/Path/ToMap, -AllMaps, and/or -MeshShapes"))
    { return 1; }

    auto FailureCount = 0;
    auto MapIndex = 0;

    for (const auto& MapPackageName : MapsToCook)
    {
        ++MapIndex;
        UE_LOG(CkJolt, Display, TEXT("CkJoltCook: map %d/%d (%s): %s"),
            MapIndex, MapsToCook.Num(), Incremental ? TEXT("incremental") : TEXT("full rebuild"), *MapPackageName);
        if (NOT DoCook_Map(MapPackageName, CookMode, PackagingExcludedLevelPackagePaths, Incremental))
        { ++FailureCount; }
    }

    if (MapsToCook.Num() > 0)
    { UE_LOG(CkJolt, Display, TEXT("CkJoltCook: %d maps processed, %d failed"), MapsToCook.Num(), FailureCount); }

    return (FailureCount == 0 && NOT MeshShapesFailed) ? 0 : 1;
}

auto
    UCk_JoltCook_Commandlet::
    DoEnsure_StreamingLevelsInWorld(
        UWorld& InWorld,
        const TArray<FString>& InExcludedLevelPackagePaths)
    -> bool
{
    auto NumAdded = 0;

    for (auto* StreamingLevel : InWorld.GetStreamingLevels())
    {
        if (ck::Is_NOT_Valid(StreamingLevel))
        { continue; }

        const auto StreamingLevelPackageName = StreamingLevel->GetWorldAssetPackageName();
        if (ck::jolt::cook::Get_IsPackageExcluded(StreamingLevelPackageName, InExcludedLevelPackagePaths))
        {
            ck::jolt::Log(TEXT("CkJoltCook: skipping excluded streaming level [{}] of map [{}]"),
                StreamingLevelPackageName, InWorld.GetOutermost()->GetName());
            continue;
        }

        auto* LoadedLevel = ck_jolt_cook_commandlet::Get_ResolvedLevel(*StreamingLevel);

        // Fail CLOSED. Cooking on a partially-loaded world writes a bake that is missing the
        // unloaded sublevels' actors, and a missing actor reads as "no collision here" at runtime —
        // silently, and indistinguishably from a correct bake. Better no new bake than a short one.
        const auto LevelIsResolved = ck::IsValid(LoadedLevel);
        CK_ENSURE_IF_NOT(LevelIsResolved,
            TEXT("CkJoltCook: streaming level [{}] of map [{}] did not load — REFUSING to cook, because a "
                 "partial world would write a bake that silently omits that sublevel's collision"),
            StreamingLevelPackageName, InWorld.GetOutermost()->GetName())
        {}

        if (NOT LevelIsResolved)
        { return false; }

        // Component registration runs against OwningWorld, which for a level resolved from its own
        // package is still that sublevel's standalone world. Repoint before adding, exactly as
        // UWorld::LoadSecondaryLevels does, or components register into the wrong world.
        LoadedLevel->OwningWorld = &InWorld;

        ck::jolt::Log(TEXT("CkJoltCook: streaming level [{}] -> [{}] actor(s)"),
            StreamingLevel->GetWorldAssetPackageName(), LoadedLevel->Actors.Num());

        if (InWorld.GetLevels().Contains(LoadedLevel))
        { continue; }

        // LoadSecondaryLevels only sets the streaming level's loaded-level pointer; the ULevel is not
        // in the world (so its components never register and the sweep cannot see its actors) until
        // it is added here.
        constexpr auto ConsiderTimeLimit = false;
        InWorld.AddToWorld(LoadedLevel, StreamingLevel->LevelTransform, ConsiderTimeLimit);
        ++NumAdded;
    }

    ck::jolt::Log(TEXT("CkJoltCook: loaded [{}] streaming level(s) for map [{}] — [{}] level(s) total"),
        NumAdded, InWorld.GetOutermost()->GetName(), InWorld.GetLevels().Num());

    // AddToWorld can decline silently (visibility budget, CanAddLoadedLevelToWorld), and a guard that
    // assumes it succeeded reproduces the very truncation it exists to prevent. Re-check the world.
    for (const auto* StreamingLevel : InWorld.GetStreamingLevels())
    {
        if (ck::Is_NOT_Valid(StreamingLevel))
        { continue; }

        const auto StreamingLevelPackageName = StreamingLevel->GetWorldAssetPackageName();
        if (ck::jolt::cook::Get_IsPackageExcluded(StreamingLevelPackageName, InExcludedLevelPackagePaths))
        { continue; }

        const auto* LoadedLevel = ck_jolt_cook_commandlet::Get_ResolvedLevel(*StreamingLevel);

        const auto LevelIsInWorld = ck::IsValid(LoadedLevel) && InWorld.GetLevels().Contains(LoadedLevel);
        CK_ENSURE_IF_NOT(LevelIsInWorld,
            TEXT("CkJoltCook: streaming level [{}] did not end up in the world after AddToWorld — REFUSING "
                 "to cook rather than write a bake missing its collision"),
            StreamingLevelPackageName)
        {}

        if (NOT LevelIsInWorld)
        { return false; }
    }

    return true;
}

auto
    UCk_JoltCook_Commandlet::
    DoCook_Map(
        const FString& InMapPackageName,
        ck::jolt::cook::ECk_Jolt_CookMode InMode,
        const TArray<FString>& InExcludedLevelPackagePaths,
        bool InIncremental)
    -> bool
{
    ck::jolt::Log(TEXT("CkJoltCook: loading map [{}]"), InMapPackageName);

    // The EDITOR's map loader, not LoadPackage. LoadPackage yields the persistent level alone, and
    // on a level-streamed map that is a nearly empty world — the cook then reports "no static
    // collision to cook" (or, worse, writes a bake missing almost every actor). LoadMap brings in
    // the streaming sublevels and registers their components, which is what the sweep needs and
    // what the editor-subsystem cook vehicle has always had for free.
    const auto MapFilename = FPackageName::LongPackageNameToFilename(
        InMapPackageName, FPackageName::GetMapPackageExtension());

    auto* World = UEditorLoadingAndSavingUtils::LoadMap(MapFilename);

    CK_ENSURE_IF_NOT(ck::IsValid(World),
        TEXT("CkJoltCook: failed to load map [{}] (file [{}])"), InMapPackageName, MapFilename)
    { return false; }

    // Validation, not loading: LoadMap should have brought every sublevel in. Anything still missing
    // means a partial world, and cooking that writes a bake that silently omits its collision.
    if (NOT DoEnsure_StreamingLevelsInWorld(*World, InExcludedLevelPackagePaths))
    { return false; }

    const auto Stats = InIncremental
        ? FCk_Jolt_WorldCooker::Cook_World_Incremental(*World, InMode, InExcludedLevelPackagePaths)
        : FCk_Jolt_WorldCooker::Cook_World(*World, InMode, InExcludedLevelPackagePaths);

    const auto FellBackToFullCook = InIncremental
        && Stats._Outcome != ck::jolt::cook::ECk_Jolt_IncrementalOutcome::Incremental;
    UE_LOG(CkJolt, Display, TEXT("CkJoltCook: completed %s: %s, %d cells written, %d actors current%s"),
        *InMapPackageName, Stats._Success ? TEXT("success") : TEXT("failed"),
        Stats._NumCellsWritten, Stats._NumActorsUpToDate,
        FellBackToFullCook ? TEXT(" (full rebuild fallback)") : TEXT(""));
    return Stats._Success;
}

// --------------------------------------------------------------------------------------------------------------------
