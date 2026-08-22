#include "CkJoltCook_Commandlet.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJoltEditor/Cook/CkJoltCook_MeshShapeCooker.h"
#include "CkJoltEditor/Cook/CkJoltCook_WorldCooker.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <AssetRegistry/IAssetRegistry.h>
#include <Engine/World.h>
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
    DoCook_Map(
        const FString& InMapPackageName,
        ck::jolt::cook::ECk_Jolt_CookMode InMode)
    -> bool
{
    ck::jolt::Log(TEXT("CkJoltCook: loading map [{}]"), InMapPackageName);

    auto* Package = LoadPackage(nullptr, *InMapPackageName, LOAD_None);

    CK_ENSURE_IF_NOT(ck::IsValid(Package),
        TEXT("CkJoltCook: failed to load map package [{}]"), InMapPackageName)
    { return false; }

    auto* World = UWorld::FindWorldInPackage(Package);

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("CkJoltCook: package [{}] contains no UWorld"), InMapPackageName)
    { return false; }

    World->AddToRoot();

    auto Cooked = false;
    {
        const auto InitializationValues = UWorld::InitializationValues{}
            .RequiresHitProxies(false)
            .ShouldSimulatePhysics(false)
            .EnableTraceCollision(false)
            .CreateNavigation(false)
            .CreateAISystem(false);

        if (NOT World->bIsWorldInitialized)
        { World->InitWorld(InitializationValues); }

        constexpr auto RerunConstructionScripts = false;
        World->UpdateWorldComponents(RerunConstructionScripts, true);

        Cooked = FCk_Jolt_WorldCooker::Cook_World(*World, InMode)._Success;
    }
    World->RemoveFromRoot();

    CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

    return Cooked;
}

// --------------------------------------------------------------------------------------------------------------------
