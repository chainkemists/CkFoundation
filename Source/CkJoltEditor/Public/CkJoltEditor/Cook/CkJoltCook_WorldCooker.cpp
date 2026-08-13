#include "CkJoltCook_WorldCooker.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJolt/StaticWorld/CkJoltBakeExtraction.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Data.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <Engine/Level.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>
#include <Misc/ScopeExit.h>
#include <UObject/Package.h>
#include <UObject/SavePackage.h>
#include <WorldPartition/WorldPartition.h>
#include <WorldPartition/WorldPartitionHelpers.h>
#include <WorldPartition/WorldPartitionActorDescInstance.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Core/Core.h>

#include <sstream>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_cook_world_cooker
{
    using namespace ck::jolt;
    using namespace ck::jolt::bake;

    struct FActorCookData
    {
        FName _ActorName;
        FSoftObjectPath _ActorPath;
        uint64 _SourceHash = 0;
        uint64 _RuntimeCheckHash = 0;
        TArray<FCk_Jolt_ExtractedBody> _Bodies;
    };

    struct FCellCookData
    {
        FIntPoint _CellId = FIntPoint::ZeroValue;
        FBox _Bounds = FBox{ForceInit};
        TArray<FActorCookData> _Actors;
    };

    // ----------------------------------------------------------------------------------------------------------------

    static auto Get_CellIdForPosition(const FVector& InPosition, float InCellSize) -> FIntPoint
    {
        return FIntPoint{
            FMath::FloorToInt32(InPosition.X / InCellSize),
            FMath::FloorToInt32(InPosition.Y / InCellSize)};
    }

    static auto DoExtract_Actor(
        const AActor& InActor,
        FCk_Jolt_ShapeCache& InShapeCache,
        const FCk_Jolt_BakeFilter& InFilter,
        TArray<FActorCookData>& OutActors)
        -> int32
    {
        auto Bodies = TArray<FCk_Jolt_ExtractedBody>{};
        ExtractActor(InActor, InShapeCache, Bodies, InFilter);

        if (Bodies.IsEmpty())
        { return 0; }

        auto& ActorData = OutActors.Emplace_GetRef();
        ActorData._ActorName = InActor.GetFName();
        ActorData._ActorPath = FSoftObjectPath{&InActor};
        ActorData._SourceHash = ComputeSourceHash(InActor, InFilter);
        ActorData._RuntimeCheckHash = ComputeRuntimeCheckHash(InActor, InFilter);
        ActorData._Bodies = MoveTemp(Bodies);

        return ActorData._Bodies.Num();
    }

    // ----------------------------------------------------------------------------------------------------------------

    static auto DoSave_Asset(UObject& InAsset) -> bool
    {
        auto* Package = InAsset.GetOutermost();

        FAssetRegistryModule::AssetCreated(&InAsset);
        Package->MarkPackageDirty();

        const auto FileName = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());

        auto SaveArgs = FSavePackageArgs{};
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

        return UPackage::SavePackage(Package, &InAsset, *FileName, SaveArgs);
    }

    template <typename T_Asset>
    static auto DoCreate_Asset(const FString& InLongPackageName, const FString& InAssetName) -> T_Asset*
    {
        auto* Package = CreatePackage(*InLongPackageName);

        CK_ENSURE_IF_NOT(Package != nullptr, TEXT("Failed to create package [{}]"), InLongPackageName)
        { return nullptr; }

        Package->FullyLoad();

        return NewObject<T_Asset>(Package, *InAssetName, RF_Public | RF_Standalone);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_WorldCooker::
    Cook_World(
        UWorld& InWorld,
        bool InDryRun)
    -> FCookStats
{
    using namespace ck_jolt_cook_world_cooker;
    using namespace ck::jolt;
    using namespace ck::jolt::bake;

    auto Stats = FCookStats{};

    // Cook vehicles (commandlet, editor subsystem) have no game world, so nothing else has
    // registered Jolt's allocator/factory/types — shape creation crashes without this.
    Request_GlobalJoltInit();
    ON_SCOPE_EXIT { Request_GlobalJoltShutdown(); };

    const auto CellSize = UCk_Utils_Jolt_ProjectSettings::Get_BakeGridCellSize();
    const auto RootPath = UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath();
    const auto MapPackageName = InWorld.PersistentLevel->GetOutermost()->GetName();

    // ---- Gather + extract -------------------------------------------------------------------------

    auto ShapeCache = FCk_Jolt_ShapeCache{};
    auto ActorCookData = TArray<FActorCookData>{};
    const auto BakeFilter = FCk_Jolt_BakeFilter::Make_FromProjectSettings();

    for (const auto& Level : InWorld.GetLevels())
    {
        if (ck::Is_NOT_Valid(Level))
        { continue; }

        for (const auto& Actor : Level->Actors)
        {
            if (ck::Is_NOT_Valid(Actor))
            { continue; }

            Stats._NumBodies += DoExtract_Actor(*Actor, ShapeCache, BakeFilter, ActorCookData);
        }
    }

    // World Partition: visit actors that are NOT currently loaded, in memory-safe batches.
    if (auto* WorldPartition = InWorld.GetWorldPartition())
    {
        auto AlreadyCooked = TSet<FName>{};
        for (const auto& ActorData : ActorCookData)
        { AlreadyCooked.Add(ActorData._ActorName); }

        FWorldPartitionHelpers::ForEachActorWithLoading(WorldPartition,
            [&](const FWorldPartitionActorDescInstance* InActorDescInstance) -> bool
            {
                const auto* Actor = InActorDescInstance->GetActor();
                if (ck::Is_NOT_Valid(Actor))
                { return true; }

                if (AlreadyCooked.Contains(Actor->GetFName()))
                { return true; }

                Stats._NumBodies += DoExtract_Actor(*Actor, ShapeCache, BakeFilter, ActorCookData);
                return true;
            });
    }

    Stats._NumActors = ActorCookData.Num();
    Stats._NumUniqueShapes = ShapeCache.Get_NumUniqueShapes();

    if (ActorCookData.IsEmpty())
    {
        ck::jolt::Log(TEXT("JoltCook: map [{}] has no static collision to cook"), MapPackageName);
        Stats._Success = true;
        return Stats;
    }

    // ---- Partition into cells ---------------------------------------------------------------------

    auto Cells = TMap<FIntPoint, FCellCookData>{};

    for (auto& ActorData : ActorCookData)
    {
        const auto CellId = Get_CellIdForPosition(ActorData._Bodies[0]._Position, CellSize);

        auto& Cell = Cells.FindOrAdd(CellId);
        Cell._CellId = CellId;

        for (const auto& Body : ActorData._Bodies)
        { Cell._Bounds += Body._Position; }

        Cell._Actors.Emplace(MoveTemp(ActorData));
    }

    Stats._NumCells = Cells.Num();

    if (InDryRun)
    {
        ck::jolt::Log(TEXT("JoltCook DRY RUN: [{}] actors, [{}] bodies, [{}] unique shapes, [{}] cells"),
            Stats._NumActors, Stats._NumBodies, Stats._NumUniqueShapes, Stats._NumCells);
        Stats._Success = true;
        return Stats;
    }

    // ---- Serialize + save cell assets --------------------------------------------------------------

    auto MapSubPath = MapPackageName;
    MapSubPath.RemoveFromStart(TEXT("/Game"));

    auto CellRefs = TArray<FCk_Jolt_CookedCellRef>{};
    auto ActorLookup = TMap<FName, FCk_Jolt_CookedActorRef>{};

    for (auto& [CellId, Cell] : Cells)
    {
        const auto CellAssetName = ck::Format_UE(TEXT("JoltCell_{}_{}"), CellId.X, CellId.Y);
        const auto CellPackageName = ck::Format_UE(TEXT("{}{}/{}"), RootPath, MapSubPath, CellAssetName);

        auto* CellAsset = DoCreate_Asset<UCk_Jolt_CookedCell_UE>(CellPackageName, CellAssetName);

        CK_ENSURE_IF_NOT(ck::IsValid(CellAsset), TEXT("Failed to create cell asset [{}]"), CellPackageName)
        { return Stats; }

        // Dedup shapes by pointer identity within the cell (the shape cache already dedups per
        // BodySetup+scale) and write ONE SaveWithChildren stream with shared maps.
        auto BlobStream = std::ostringstream{};
        auto StreamWrapper = JPH::StreamOutWrapper{BlobStream};
        auto ShapeToId = JPH::Shape::ShapeToIDMap{};
        auto MaterialToId = JPH::Shape::MaterialToIDMap{};

        auto ShapeToIndex = TMap<const JPH::Shape*, int32>{};
        auto ActorGroups = TArray<FCk_Jolt_CookedActorGroup>{};

        for (const auto& ActorData : Cell._Actors)
        {
            auto Group = FCk_Jolt_CookedActorGroup{};
            Group.Set_SourceActorName(ActorData._ActorName);
            Group.Set_SourceActorPath(ActorData._ActorPath);
            Group.Set_SourceHash(ActorData._SourceHash);
            Group.Set_RuntimeCheckHash(ActorData._RuntimeCheckHash);

            auto Records = TArray<FCk_Jolt_CookedBodyRecord>{};

            for (const auto& Body : ActorData._Bodies)
            {
                if (Body._Shape == nullptr)
                { continue; }

                auto ShapeIndex = int32{INDEX_NONE};

                if (const auto* Existing = ShapeToIndex.Find(Body._Shape.GetPtr()))
                { ShapeIndex = *Existing; }
                else
                {
                    Body._Shape->SaveWithChildren(StreamWrapper, ShapeToId, MaterialToId);
                    ShapeIndex = ShapeToIndex.Num();
                    ShapeToIndex.Add(Body._Shape.GetPtr(), ShapeIndex);
                }

                auto Record = FCk_Jolt_CookedBodyRecord{};
                Record.Set_ShapeIndex(ShapeIndex);
                Record.Set_Position(Body._Position);
                Record.Set_Rotation(Body._Rotation);
                Record.Set_Signature(Body._Signature);
                Record.Set_Friction(Body._Friction);
                Record.Set_Restitution(Body._Restitution);
                Record.Set_SurfaceType(Body._SurfaceType);
                Records.Emplace(MoveTemp(Record));
            }

            Group.Set_Bodies(MoveTemp(Records));

            ActorLookup.Add(ActorData._ActorName,
                FCk_Jolt_CookedActorRef{}.Set_CellIndex(CellRefs.Num()).Set_GroupIndex(ActorGroups.Num()));

            ActorGroups.Emplace(MoveTemp(Group));
        }

        const auto BlobString = BlobStream.str();
        auto Blob = TArray<uint8>{};
        Blob.Append(reinterpret_cast<const uint8*>(BlobString.data()), BlobString.size());

        CellAsset->Set_CookVersion(CookVersion_Current);
        CellAsset->Set_JoltVersionId(static_cast<uint32>(JPH_VERSION_ID));
        CellAsset->Set_CellId(CellId);
        CellAsset->Set_ShapeBlob(MoveTemp(Blob));
        CellAsset->Set_ShapeCount(ShapeToIndex.Num());
        CellAsset->Set_ActorGroups(MoveTemp(ActorGroups));

        CK_ENSURE_IF_NOT(DoSave_Asset(*CellAsset), TEXT("Failed to SAVE cell asset [{}]"), CellPackageName)
        { return Stats; }

        auto CellRef = FCk_Jolt_CookedCellRef{};
        CellRef.Set_CellId(CellId);
        CellRef.Set_Bounds(Cell._Bounds);
        CellRef.Set_CellAsset(TSoftObjectPtr<UCk_Jolt_CookedCell_UE>{CellAsset});
        CellRefs.Emplace(MoveTemp(CellRef));
    }

    // ---- Index asset --------------------------------------------------------------------------------

    const auto IndexPackageName = ck::Format_UE(TEXT("{}{}/JoltIndex"), RootPath, MapSubPath);
    auto* IndexAsset = DoCreate_Asset<UCk_Jolt_CookedWorldIndex_UE>(IndexPackageName, TEXT("JoltIndex"));

    CK_ENSURE_IF_NOT(ck::IsValid(IndexAsset), TEXT("Failed to create index asset [{}]"), IndexPackageName)
    { return Stats; }

    IndexAsset->Set_CookVersion(CookVersion_Current);
    IndexAsset->Set_JoltVersionId(static_cast<uint32>(JPH_VERSION_ID));
    IndexAsset->Set_SourceMapPackage(FName{*MapPackageName});
    IndexAsset->Set_BakeFilterHash(BakeFilter.ComputeHash());
    IndexAsset->Set_Cells(MoveTemp(CellRefs));
    IndexAsset->Set_ActorLookup(MoveTemp(ActorLookup));

    CK_ENSURE_IF_NOT(DoSave_Asset(*IndexAsset), TEXT("Failed to SAVE index asset [{}]"), IndexPackageName)
    { return Stats; }

    ck::jolt::Log(TEXT("JoltCook: cooked map [{}] — [{}] actors, [{}] bodies, [{}] unique shapes, [{}] cells -> [{}]"),
        MapPackageName, Stats._NumActors, Stats._NumBodies, Stats._NumUniqueShapes, Stats._NumCells, IndexPackageName);

    Stats._Success = true;
    return Stats;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_WorldCooker::
    Validate_World(
        UWorld& InWorld)
    -> FCookStats
{
    using namespace ck_jolt_cook_world_cooker;
    using namespace ck::jolt;
    using namespace ck::jolt::bake;

    auto Stats = FCookStats{};

    const auto RootPath = UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath();
    const auto MapPackageName = InWorld.PersistentLevel->GetOutermost()->GetName();
    const auto IndexPath = Get_CookedIndexAssetPath(RootPath, MapPackageName);

    const auto* Index = LoadObject<UCk_Jolt_CookedWorldIndex_UE>(nullptr, *IndexPath);

    if (ck::Is_NOT_Valid(Index))
    {
        ck::jolt::Warning(TEXT("JoltCook validate: no cooked index at [{}] — map was never cooked"), IndexPath);
        return Stats;
    }

    auto StaleCount = 0;
    const auto BakeFilter = FCk_Jolt_BakeFilter::Make_FromProjectSettings();

    if (Index->Get_BakeFilterHash() != BakeFilter.ComputeHash())
    {
        ck::jolt::Warning(TEXT("JoltCook validate: cooked index for [{}] was baked under DIFFERENT bake-filter "
            "settings — the whole map needs a re-cook"), MapPackageName);
        return Stats;
    }

    for (const auto& Level : InWorld.GetLevels())
    {
        if (ck::Is_NOT_Valid(Level))
        { continue; }

        for (const auto& Actor : Level->Actors)
        {
            if (ck::Is_NOT_Valid(Actor))
            { continue; }

            const auto* ActorRef = Index->Get_ActorLookup().Find(Actor->GetFName());
            if (ActorRef == nullptr)
            { continue; }

            ++Stats._NumActors;

            const auto& CellRef = Index->Get_Cells()[ActorRef->Get_CellIndex()];
            const auto* CellAsset = CellRef.Get_CellAsset().LoadSynchronous();
            if (ck::Is_NOT_Valid(CellAsset, ck::IsValid_Policy_NullptrOnly{}))
            { ++StaleCount; continue; }

            const auto& Group = CellAsset->Get_ActorGroups()[ActorRef->Get_GroupIndex()];

            if (ComputeSourceHash(*Actor, BakeFilter) == Group.Get_SourceHash())
            { ++Stats._NumActorsUpToDate; }
            else
            {
                ++StaleCount;
                ck::jolt::Warning(TEXT("JoltCook validate: actor [{}] is STALE in the cooked data"),
                    Actor->GetFName());
            }
        }
    }

    ck::jolt::Log(TEXT("JoltCook validate: [{}] cooked actors checked, [{}] up to date, [{}] stale"),
        Stats._NumActors, Stats._NumActorsUpToDate, StaleCount);

    Stats._Success = StaleCount == 0;
    return Stats;
}

// --------------------------------------------------------------------------------------------------------------------
