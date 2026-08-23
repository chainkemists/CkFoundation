#include "CkJoltCook_Commandlet.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJoltEditor/Cook/CkJoltCook_MeshShapeCooker.h"
#include "CkJoltEditor/Cook/CkJoltCook_WorldCooker.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <AssetRegistry/IAssetRegistry.h>
#include <Engine/Level.h>
#include <Engine/LevelStreaming.h>
#include <Engine/World.h>
#include <FileHelpers.h>
#include <Misc/PackageName.h>
#include <UObject/Package.h>
#include <WorldPartition/WorldPartition.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_cook_commandlet
{
    // The cooked-data root must be force-cooked into packaged builds — nothing hard-references
    // the assets (index is found by path convention), so a missing entry ships builds with NO
    // Jolt static world. Loud, with the exact line to add.
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

    ck_jolt_cook_commandlet::DoEnsure_AlwaysCookEntry();

    // Per-mesh shape sweep (-MeshShapes): independent of any map. May be combined with -Map/-AllMaps
    // or run alone.
    const auto CookMeshShapes = Switches.Contains(TEXT("MeshShapes"));
    auto MeshShapesFailed = false;

    if (CookMeshShapes)
    {
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().SearchAllAssets(true);
        MeshShapesFailed = NOT FCk_Jolt_MeshShapeCooker::Cook_MeshShapes(CookMode)._Success;
    }

    auto MapsToCook = TArray<FString>{};

    if (const auto* SingleMap = ParamsMap.Find(TEXT("Map")))
    { MapsToCook.Add(*SingleMap); }
    else if (Switches.Contains(TEXT("AllMaps")))
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

    for (const auto& MapPackageName : MapsToCook)
    {
        if (NOT DoCook_Map(MapPackageName, CookMode))
        { ++FailureCount; }
    }

    if (MapsToCook.Num() > 0)
    { ck::jolt::Log(TEXT("CkJoltCook: [{}] maps processed, [{}] failed"), MapsToCook.Num(), FailureCount); }

    return (FailureCount == 0 && NOT MeshShapesFailed) ? 0 : 1;
}

auto
    UCk_JoltCook_Commandlet::
    DoEnsure_StreamingLevelsInWorld(
        UWorld& InWorld)
    -> bool
{
    auto NumAdded = 0;

    for (auto* StreamingLevel : InWorld.GetStreamingLevels())
    {
        if (ck::Is_NOT_Valid(StreamingLevel))
        { continue; }

        auto* LoadedLevel = StreamingLevel->GetLoadedLevel();

        // LoadSecondaryLevels leaves the loaded-level pointer unset for a sublevel whose package was
        // already in memory; the ULevel is perfectly good, so resolve it from the package instead of
        // refusing the whole map.
        if (ck::Is_NOT_Valid(LoadedLevel))
        {
            if (auto* LevelPackage = FindPackage(nullptr, *StreamingLevel->GetWorldAssetPackageName()))
            {
                if (auto* LevelWorld = UWorld::FindWorldInPackage(LevelPackage))
                {
                    LoadedLevel = LevelWorld->PersistentLevel;

                    // Component registration runs against OwningWorld, which here is still the
                    // sublevel's own standalone world. Repoint before adding, exactly as
                    // UWorld::LoadSecondaryLevels does, or components register into the wrong world.
                    if (ck::IsValid(LoadedLevel))
                    { LoadedLevel->OwningWorld = &InWorld; }
                }
            }
        }

        // Fail CLOSED. Cooking on a partially-loaded world writes a bake that is missing the
        // unloaded sublevels' actors, and a missing actor reads as "no collision here" at runtime —
        // silently, and indistinguishably from a correct bake. Better no new bake than a short one.
        CK_ENSURE_IF_NOT(ck::IsValid(LoadedLevel),
            TEXT("CkJoltCook: streaming level [{}] of map [{}] did not load — REFUSING to cook, because a "
                 "partial world would write a bake that silently omits that sublevel's collision"),
            StreamingLevel->GetWorldAssetPackageName(), InWorld.GetOutermost()->GetName())
        { return false; }

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

        const auto* LoadedLevel = StreamingLevel->GetLoadedLevel();

        CK_ENSURE_IF_NOT(ck::IsValid(LoadedLevel) && InWorld.GetLevels().Contains(LoadedLevel),
            TEXT("CkJoltCook: streaming level [{}] did not end up in the world after AddToWorld — REFUSING "
                 "to cook rather than write a bake missing its collision"),
            StreamingLevel->GetWorldAssetPackageName())
        { return false; }
    }

    return true;
}

auto
    UCk_JoltCook_Commandlet::
    DoCook_Map(
        const FString& InMapPackageName,
        ck::jolt::cook::ECk_Jolt_CookMode InMode)
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
    if (NOT DoEnsure_StreamingLevelsInWorld(*World))
    { return false; }

    return FCk_Jolt_WorldCooker::Cook_World(*World, InMode)._Success;
}

// --------------------------------------------------------------------------------------------------------------------
