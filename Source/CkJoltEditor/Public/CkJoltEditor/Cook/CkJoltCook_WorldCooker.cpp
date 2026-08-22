#include "CkJoltCook_WorldCooker.h"

#include "CkJoltCook_AssetSave.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJolt/StaticWorld/CkJoltBakeExtraction.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Data.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h"

#include <Engine/Level.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>
#include <Misc/ScopeExit.h>
#include <UObject/Package.h>
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
    using namespace ck::jolt::cook;

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
        TArray<FActorCookData> _Actors;
    };

    struct FWrittenCell
    {
        FCk_Jolt_CookedCellRef _CellRef;
        TArray<FName> _ActorNamesByGroupIndex;
        int32 _NumUniqueShapes = 0;
        bool _Success = false;
    };

    // ----------------------------------------------------------------------------------------------------------------

    static auto Get_CellIdForPosition(const FVector& InPosition, float InCellSize) -> FIntPoint
    {
        return FIntPoint{
            FMath::FloorToInt32(InPosition.X / InCellSize),
            FMath::FloorToInt32(InPosition.Y / InCellSize)};
    }

    static auto Get_CellIdForActor(const FActorCookData& InActorData, float InCellSize) -> FIntPoint
    {
        return Get_CellIdForPosition(InActorData._Bodies[0]._Position, InCellSize);
    }

    static auto Get_MapSubPath(const FString& InMapPackageName) -> FString
    {
        auto MapSubPath = InMapPackageName;
        MapSubPath.RemoveFromStart(TEXT("/Game"));
        return MapSubPath;
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
        return ck::jolt::cook::Save_CookedAsset(InAsset);
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

    // ----------------------------------------------------------------------------------------------------------------

    /// One SaveWithChildren stream with shared maps, so a shape used by N bodies is stored once and
    /// restored as one shared JPH::Ref.
    static auto DoWrite_Cell(
        const FCellCookData& InCell,
        const FString& InRootPath,
        const FString& InMapSubPath)
        -> FWrittenCell
    {
        auto Written = FWrittenCell{};

        const auto CellAssetName = ck::Format_UE(TEXT("JoltCell_{}_{}"), InCell._CellId.X, InCell._CellId.Y);
        const auto CellPackageName = ck::Format_UE(TEXT("{}{}/{}"), InRootPath, InMapSubPath, CellAssetName);

        auto* CellAsset = DoCreate_Asset<UCk_Jolt_CookedCell_UE>(CellPackageName, CellAssetName);

        CK_ENSURE_IF_NOT(ck::IsValid(CellAsset), TEXT("Failed to create cell asset [{}]"), CellPackageName)
        { return Written; }

        auto BlobStream = std::ostringstream{};
        auto StreamWrapper = JPH::StreamOutWrapper{BlobStream};
        auto ShapeToId = JPH::Shape::ShapeToIDMap{};
        auto MaterialToId = JPH::Shape::MaterialToIDMap{};

        auto ShapeToIndex = TMap<const JPH::Shape*, int32>{};
        auto ActorGroups = TArray<FCk_Jolt_CookedActorGroup>{};
        auto Bounds = FBox{ForceInit};

        for (const auto& ActorData : InCell._Actors)
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

                Bounds += Body._Position;

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

            Written._ActorNamesByGroupIndex.Emplace(ActorData._ActorName);
            ActorGroups.Emplace(MoveTemp(Group));
        }

        const auto BlobString = BlobStream.str();
        auto Blob = TArray<uint8>{};
        Blob.Append(reinterpret_cast<const uint8*>(BlobString.data()), BlobString.size());

        CellAsset->Set_CookVersion(CookVersion_Current);
        CellAsset->Set_JoltVersionId(static_cast<uint32>(JPH_VERSION_ID));
        CellAsset->Set_CellId(InCell._CellId);
        CellAsset->Set_ShapeBlob(MoveTemp(Blob));
        CellAsset->Set_ShapeCount(ShapeToIndex.Num());
        CellAsset->Set_ActorGroups(MoveTemp(ActorGroups));

        const auto CellSaved = DoSave_Asset(*CellAsset);
        CK_ENSURE_IF_NOT(CellSaved, TEXT("Failed to SAVE cell asset [{}]"), CellPackageName)
        { return Written; }

        Written._CellRef.Set_CellId(InCell._CellId);
        Written._CellRef.Set_Bounds(Bounds);
        Written._CellRef.Set_CellAsset(TSoftObjectPtr<UCk_Jolt_CookedCell_UE>{CellAsset});
        Written._NumUniqueShapes = ShapeToIndex.Num();
        Written._Success = true;

        return Written;
    }

    // ----------------------------------------------------------------------------------------------------------------

    static auto DoWrite_Index(
        const FString& InRootPath,
        const FString& InMapSubPath,
        const FString& InMapPackageName,
        const FCk_Jolt_BakeFilter& InFilter,
        TArray<FCk_Jolt_CookedCellRef> InCellRefs,
        TMap<FName, FCk_Jolt_CookedActorRef> InActorLookup,
        FString& OutIndexPackageName)
        -> bool
    {
        OutIndexPackageName = ck::Format_UE(TEXT("{}{}/JoltIndex"), InRootPath, InMapSubPath);

        auto* IndexAsset = DoCreate_Asset<UCk_Jolt_CookedWorldIndex_UE>(OutIndexPackageName, TEXT("JoltIndex"));

        CK_ENSURE_IF_NOT(ck::IsValid(IndexAsset), TEXT("Failed to create index asset [{}]"), OutIndexPackageName)
        { return false; }

        IndexAsset->Set_CookVersion(CookVersion_Current);
        IndexAsset->Set_JoltVersionId(static_cast<uint32>(JPH_VERSION_ID));
        IndexAsset->Set_SourceMapPackage(FName{*InMapPackageName});
        IndexAsset->Set_BakeFilterHash(InFilter.ComputeHash());
        IndexAsset->Set_Cells(MoveTemp(InCellRefs));
        IndexAsset->Set_ActorLookup(MoveTemp(InActorLookup));

        const auto IndexSaved = DoSave_Asset(*IndexAsset);
        CK_ENSURE_IF_NOT(IndexSaved, TEXT("Failed to SAVE index asset [{}]"), OutIndexPackageName)
        { return false; }

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    /// Mirrors the runtime's cell load: shapes come back in stream order, so a record's ShapeIndex
    /// resolves against the result.
    static auto DoRestore_CellShapes(
        const UCk_Jolt_CookedCell_UE& InCellAsset,
        TArray<JPH::Ref<JPH::Shape>>& OutShapes)
        -> bool
    {
        const auto& Blob = InCellAsset.Get_ShapeBlob();

        auto BlobStream = std::istringstream{
            std::string{reinterpret_cast<const char*>(Blob.GetData()), static_cast<size_t>(Blob.Num())}};
        auto StreamWrapper = JPH::StreamInWrapper{BlobStream};

        auto IdToShape = JPH::Shape::IDToShapeMap{};
        auto IdToMaterial = JPH::Shape::IDToMaterialMap{};

        OutShapes.Reset();
        OutShapes.Reserve(InCellAsset.Get_ShapeCount());

        for (auto Index = 0; Index < InCellAsset.Get_ShapeCount(); ++Index)
        {
            const auto Result = JPH::Shape::sRestoreWithChildren(StreamWrapper, IdToShape, IdToMaterial);

            CK_ENSURE_IF_NOT(Result.IsValid(),
                TEXT("Cooked Jolt cell [{}] shape [{}] failed to restore during an incremental re-cook: [{}]"),
                InCellAsset.Get_CellId(), Index, FString{Result.GetError().c_str()})
            { return false; }

            OutShapes.Emplace(Result.Get());
        }

        return true;
    }

    static auto DoRehydrate_CookedActor(
        const FCk_Jolt_CookedActorGroup& InGroup,
        const TArray<JPH::Ref<JPH::Shape>>& InCellShapes)
        -> TOptional<FActorCookData>
    {
        auto ActorData = FActorCookData{};
        ActorData._ActorName = InGroup.Get_SourceActorName();
        ActorData._ActorPath = InGroup.Get_SourceActorPath();
        ActorData._SourceHash = InGroup.Get_SourceHash();
        ActorData._RuntimeCheckHash = InGroup.Get_RuntimeCheckHash();

        for (const auto& Record : InGroup.Get_Bodies())
        {
            const auto ShapeIndexIsValid = InCellShapes.IsValidIndex(Record.Get_ShapeIndex());
            CK_ENSURE_IF_NOT(ShapeIndexIsValid,
                TEXT("Cooked body record for actor [{}] references shape [{}] out of range [{}] — the cell "
                     "cannot be re-cooked without losing this actor's collision"),
                InGroup.Get_SourceActorName(), Record.Get_ShapeIndex(), InCellShapes.Num())
            { return {}; }

            auto Body = FCk_Jolt_ExtractedBody{};
            Body._Shape = InCellShapes[Record.Get_ShapeIndex()];
            Body._Position = Record.Get_Position();
            Body._Rotation = Record.Get_Rotation();
            Body._Signature = Record.Get_Signature();
            Body._Friction = Record.Get_Friction();
            Body._Restitution = Record.Get_Restitution();
            Body._SurfaceType = Record.Get_SurfaceType();

            ActorData._Bodies.Emplace(MoveTemp(Body));
        }

        if (ActorData._Bodies.IsEmpty())
        { return {}; }

        return ActorData;
    }

    // ----------------------------------------------------------------------------------------------------------------

    static auto Get_LoadedLevelPackages(const UWorld& InWorld) -> TSet<FName>
    {
        const auto LoadedLevels = ck::algo::Filter(InWorld.GetLevels(),
            [](const ULevel* InLevel) { return ck::IsValid(InLevel); });

        return ck::algo::Transform<TSet<FName>>(LoadedLevels,
            [](const ULevel* InLevel) { return InLevel->GetOutermost()->GetFName(); });
    }

    static auto Get_LevelPackageOfCookedActor(const FCk_Jolt_CookedActorGroup& InGroup) -> FName
    {
        // /Game/Maps/Map_Gameplay.Map_Gameplay:PersistentLevel.Actor_3 -> /Game/Maps/Map_Gameplay
        const auto AssetPath = InGroup.Get_SourceActorPath().GetLongPackageName();
        return AssetPath.IsEmpty() ? FName{} : FName{*AssetPath};
    }

    /// Their geometry exists nowhere but the old blob, so a re-cook that rebuilt this cell from the
    /// world alone would delete it — the same silent truncation the incremental path exists to
    /// avoid, just cell-scoped. Unset = a restore failed and the cell must not be written.
    static auto DoCarryOver_ActorsInUnloadedLevels(
        const UCk_Jolt_CookedCell_UE& InCookedCell,
        const TMap<FName, TObjectPtr<AActor>>& InPresentActors,
        const TSet<FName>& InLoadedLevelPackages,
        TArray<FActorCookData>& OutActors)
        -> TOptional<int32>
    {
        const auto Get_NeedsCarryOver = [&](const FCk_Jolt_CookedActorGroup& InGroup)
        {
            return NOT InPresentActors.Contains(InGroup.Get_SourceActorName())
                && NOT InLoadedLevelPackages.Contains(Get_LevelPackageOfCookedActor(InGroup));
        };

        const auto Stranded = ck::algo::Filter(InCookedCell.Get_ActorGroups(), Get_NeedsCarryOver);

        if (Stranded.IsEmpty())
        { return 0; }

        auto RestoredShapes = TArray<JPH::Ref<JPH::Shape>>{};
        if (NOT DoRestore_CellShapes(InCookedCell, RestoredShapes))
        { return {}; }

        auto NumBodies = 0;

        for (const auto& Group : Stranded)
        {
            auto Rehydrated = DoRehydrate_CookedActor(Group, RestoredShapes);
            if (NOT Rehydrated.IsSet())
            { return {}; }

            NumBodies += Rehydrated->_Bodies.Num();
            OutActors.Emplace(MoveTemp(Rehydrated.GetValue()));
        }

        return NumBodies;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_WorldCooker::
    Cook_World(
        UWorld& InWorld,
        ck::jolt::cook::ECk_Jolt_CookMode InMode)
    -> FCookStats
{
    using namespace ck_jolt_cook_world_cooker;
    using namespace ck::jolt;
    using namespace ck::jolt::bake;
    using namespace ck::jolt::cook;

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
        const auto CellId = Get_CellIdForActor(ActorData, CellSize);

        auto& Cell = Cells.FindOrAdd(CellId);
        Cell._CellId = CellId;
        Cell._Actors.Emplace(MoveTemp(ActorData));
    }

    Stats._NumCells = Cells.Num();

    if (InMode == ECk_Jolt_CookMode::DryRun)
    {
        ck::jolt::Log(TEXT("JoltCook DRY RUN: [{}] actors, [{}] bodies, [{}] unique shapes, [{}] cells"),
            Stats._NumActors, Stats._NumBodies, Stats._NumUniqueShapes, Stats._NumCells);
        Stats._Success = true;
        return Stats;
    }

    // ---- Serialize + save cell assets --------------------------------------------------------------

    const auto MapSubPath = Get_MapSubPath(MapPackageName);

    auto CellRefs = TArray<FCk_Jolt_CookedCellRef>{};
    auto ActorLookup = TMap<FName, FCk_Jolt_CookedActorRef>{};

    for (const auto& [CellId, Cell] : Cells)
    {
        const auto Written = DoWrite_Cell(Cell, RootPath, MapSubPath);

        if (NOT Written._Success)
        { return Stats; }

        const auto CellIndex = CellRefs.Num();

        for (auto GroupIndex = 0; GroupIndex < Written._ActorNamesByGroupIndex.Num(); ++GroupIndex)
        {
            ActorLookup.Add(Written._ActorNamesByGroupIndex[GroupIndex],
                FCk_Jolt_CookedActorRef{}.Set_CellIndex(CellIndex).Set_GroupIndex(GroupIndex));
        }

        CellRefs.Emplace(Written._CellRef);
        ++Stats._NumCellsWritten;
    }

    // ---- Index asset --------------------------------------------------------------------------------

    auto IndexPackageName = FString{};
    if (NOT DoWrite_Index(RootPath, MapSubPath, MapPackageName, BakeFilter,
        MoveTemp(CellRefs), MoveTemp(ActorLookup), IndexPackageName))
    { return Stats; }

    ck::jolt::Log(TEXT("JoltCook: cooked map [{}] — [{}] actors, [{}] bodies, [{}] unique shapes, [{}] cells -> [{}]"),
        MapPackageName, Stats._NumActors, Stats._NumBodies, Stats._NumUniqueShapes, Stats._NumCells, IndexPackageName);

    Stats._Success = true;
    return Stats;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_WorldCooker::
    Cook_World_Incremental(
        UWorld& InWorld,
        ck::jolt::cook::ECk_Jolt_CookMode InMode)
    -> FCookStats
{
    using namespace ck_jolt_cook_world_cooker;
    using namespace ck::jolt;
    using namespace ck::jolt::bake;
    using namespace ck::jolt::cook;

    const auto RootPath = UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath();
    const auto MapPackageName = InWorld.PersistentLevel->GetOutermost()->GetName();

    const auto DoFullCook = [&](ECk_Jolt_IncrementalOutcome InOutcome, const FString& InReason) -> FCookStats
    {
        ck::jolt::Log(TEXT("JoltCook: incremental cook of [{}] declined ({}) — running a FULL cook"),
            MapPackageName, InReason);

        auto FullStats = Cook_World(InWorld, InMode);
        FullStats._Outcome = InOutcome;
        return FullStats;
    };

    // ForEachActorWithLoading releases each actor once its batch is done, so a World Partition world
    // cannot be revisited to re-extract the actors that share a dirty cell with a changed one.
    if (InWorld.GetWorldPartition() != nullptr)
    { return DoFullCook(ECk_Jolt_IncrementalOutcome::FullCook_WorldPartition, TEXT("World Partition world")); }

    const auto IndexPath = Get_CookedIndexAssetPath(RootPath, MapPackageName);
    const auto* Index = LoadObject<UCk_Jolt_CookedWorldIndex_UE>(nullptr, *IndexPath, nullptr,
        LOAD_NoWarn | LOAD_Quiet);

    if (ck::Is_NOT_Valid(Index))
    { return DoFullCook(ECk_Jolt_IncrementalOutcome::FullCook_NoExistingIndex, TEXT("map was never cooked")); }

    auto Stats = FCookStats{};

    Request_GlobalJoltInit();
    ON_SCOPE_EXIT { Request_GlobalJoltShutdown(); };

    const auto CellSize = UCk_Utils_Jolt_ProjectSettings::Get_BakeGridCellSize();
    const auto BakeFilter = FCk_Jolt_BakeFilter::Make_FromProjectSettings();

    const auto VersionsMatch = Index->Get_CookVersion() == CookVersion_Current
        && Index->Get_JoltVersionId() == static_cast<uint32>(JPH_VERSION_ID);

    if (NOT VersionsMatch)
    { return DoFullCook(ECk_Jolt_IncrementalOutcome::FullCook_ContractDrift, TEXT("cook/Jolt version drift")); }

    if (Index->Get_BakeFilterHash() != BakeFilter.ComputeHash())
    { return DoFullCook(ECk_Jolt_IncrementalOutcome::FullCook_ContractDrift, TEXT("bake-filter settings changed")); }

    // ---- Flatten the cooked state -----------------------------------------------------------------

    const auto& Cells = Index->Get_Cells();

    auto CookedCellAssets = TArray<const UCk_Jolt_CookedCell_UE*>{};
    CookedCellAssets.Reserve(Cells.Num());

    for (const auto& CellRef : Cells)
    {
        const auto* CellAsset = CellRef.Get_CellAsset().LoadSynchronous();

        if (ck::Is_NOT_Valid(CellAsset))
        {
            return DoFullCook(ECk_Jolt_IncrementalOutcome::FullCook_ContractDrift,
                ck::Format_UE(TEXT("cell [{}] failed to load"), CellRef.Get_CellId()));
        }

        CookedCellAssets.Emplace(CellAsset);
    }

    auto PlanInput = FCk_Jolt_IncrementalPlanInput{};
    PlanInput._LoadedLevelPackages = Get_LoadedLevelPackages(InWorld);

    for (auto CellIndex = 0; CellIndex < Cells.Num(); ++CellIndex)
    {
        for (const auto& Group : CookedCellAssets[CellIndex]->Get_ActorGroups())
        {
            auto CookedActor = FCk_Jolt_IncrementalCookedActor{};
            CookedActor._ActorName = Group.Get_SourceActorName();
            CookedActor._SourceHash = Group.Get_SourceHash();
            CookedActor._CellId = Cells[CellIndex].Get_CellId();
            CookedActor._OwningLevelPackage = Get_LevelPackageOfCookedActor(Group);

            PlanInput._Cooked.Emplace(MoveTemp(CookedActor));
        }
    }

    auto CookedCellIdToIndex = TMap<FIntPoint, int32>{};
    for (auto CellIndex = 0; CellIndex < Cells.Num(); ++CellIndex)
    { CookedCellIdToIndex.Add(Cells[CellIndex].Get_CellId(), CellIndex); }

    auto CookedByName = TMap<FName, const FCk_Jolt_IncrementalCookedActor*>{};
    ck::algo::ForEach(PlanInput._Cooked, [&](const FCk_Jolt_IncrementalCookedActor& InCooked)
    {
        CookedByName.Add(InCooked._ActorName, &InCooked);
    });

    // ---- Sweep the world ---------------------------------------------------------------------------

    auto ShapeCache = FCk_Jolt_ShapeCache{};
    auto PresentActors = TMap<FName, TObjectPtr<AActor>>{};

    for (const auto& Level : InWorld.GetLevels())
    {
        if (ck::Is_NOT_Valid(Level))
        { continue; }

        for (const auto& Actor : Level->Actors)
        {
            if (ck::Is_NOT_Valid(Actor))
            { continue; }

            const auto ActorName = Actor->GetFName();
            const auto SourceHash = ComputeSourceHash(*Actor, BakeFilter);

            auto Present = FCk_Jolt_IncrementalPresentActor{};
            Present._ActorName = ActorName;
            Present._SourceHash = SourceHash;

            const auto* const* CookedActor = CookedByName.Find(ActorName);
            const auto IsUnchanged = CookedActor != nullptr && (*CookedActor)->_SourceHash == SourceHash;

            if (IsUnchanged)
            {
                // The hash covers the quantized world transform, so it cannot have changed cells.
                Present._CurrentCellId = (*CookedActor)->_CellId;
                Present._HasBodies = true;
            }
            else
            {
                auto Extracted = TArray<FActorCookData>{};
                DoExtract_Actor(*Actor, ShapeCache, BakeFilter, Extracted);

                Present._HasBodies = Extracted.Num() > 0;

                if (Present._HasBodies)
                { Present._CurrentCellId = Get_CellIdForActor(Extracted[0], CellSize); }
            }

            if (Present._HasBodies)
            { ++Stats._NumActors; }

            PresentActors.Add(ActorName, Actor);
            PlanInput._Present.Emplace(MoveTemp(Present));
        }
    }

    const auto Plan = ComputeIncrementalPlan(PlanInput);

    Stats._NumActorsUpToDate = Plan._NumUnchangedActors;
    Stats._NumCells = Cells.Num();
    Stats._Outcome = ECk_Jolt_IncrementalOutcome::Incremental;

    if (Plan._DirtyCellIds.IsEmpty())
    {
        ck::jolt::Log(TEXT("JoltCook incremental: map [{}] is up to date — [{}] actors checked, [{}] preserved "
            "in unloaded levels, nothing rewritten"),
            MapPackageName, Plan._NumUnchangedActors, Plan._NumPreservedUnloadedActors);
        Stats._Success = true;
        return Stats;
    }

    if (InMode == ECk_Jolt_CookMode::DryRun)
    {
        ck::jolt::Log(TEXT("JoltCook incremental DRY RUN: map [{}] — [{}] changed, [{}] added, [{}] removed, "
            "[{}] unchanged, [{}] preserved in unloaded levels -> [{}] of [{}] cells would be rewritten"),
            MapPackageName, Plan._NumChangedActors, Plan._NumAddedActors, Plan._RemovedActorNames.Num(),
            Plan._NumUnchangedActors, Plan._NumPreservedUnloadedActors, Plan._DirtyCellIds.Num(), Cells.Num());
        Stats._Success = true;
        return Stats;
    }

    // ---- Rebuild the dirty cells -------------------------------------------------------------------

    auto DirtyCellCookData = TMap<FIntPoint, FCellCookData>{};

    for (const auto& DirtyCellId : Plan._DirtyCellIds)
    {
        auto& Cell = DirtyCellCookData.Add(DirtyCellId);
        Cell._CellId = DirtyCellId;
    }

    for (const auto& Present : PlanInput._Present)
    {
        if (NOT Present._HasBodies)
        { continue; }

        auto* Cell = DirtyCellCookData.Find(Present._CurrentCellId);
        if (Cell == nullptr)
        { continue; }

        const auto* Actor = PresentActors.Find(Present._ActorName);
        if (Actor == nullptr || ck::Is_NOT_Valid(*Actor))
        { continue; }

        Stats._NumBodies += DoExtract_Actor(**Actor, ShapeCache, BakeFilter, Cell->_Actors);
    }

    for (const auto& DirtyCellId : Plan._DirtyCellIds)
    {
        const auto* CookedCellIndex = CookedCellIdToIndex.Find(DirtyCellId);
        if (CookedCellIndex == nullptr)
        { continue; }

        const auto CarriedOver = DoCarryOver_ActorsInUnloadedLevels(
            *CookedCellAssets[*CookedCellIndex], PresentActors, PlanInput._LoadedLevelPackages,
            DirtyCellCookData[DirtyCellId]._Actors);

        if (NOT CarriedOver.IsSet())
        { return Stats; }

        Stats._NumBodies += CarriedOver.GetValue();
    }

    const auto MapSubPath = Get_MapSubPath(MapPackageName);

    auto RemapInput = FCk_Jolt_IndexRemapInput{};
    RemapInput._DirtyCellIds = Plan._DirtyCellIds;
    RemapInput._ExistingActorLookup = Index->Get_ActorLookup();
    RemapInput._ExistingCellIdsByCellIndex = ck::algo::Transform<TArray<FIntPoint>>(Cells,
        [](const FCk_Jolt_CookedCellRef& InCellRef) { return InCellRef.Get_CellId(); });

    auto FreshCellRefs = TMap<FIntPoint, FCk_Jolt_CookedCellRef>{};

    for (const auto& [CellId, Cell] : DirtyCellCookData)
    {
        if (Cell._Actors.IsEmpty())
        {
            ck::jolt::Warning(TEXT("JoltCook incremental: cell [{}] of map [{}] no longer holds any baked actor "
                "— it is dropped from the index and its cooked asset is now ORPHANED; delete it by hand"),
                CellId, MapPackageName);
            continue;
        }

        const auto Written = DoWrite_Cell(Cell, RootPath, MapSubPath);

        if (NOT Written._Success)
        { return Stats; }

        FreshCellRefs.Add(CellId, Written._CellRef);
        RemapInput._WrittenCellIds.Emplace(CellId);
        RemapInput._WrittenActorNamesByCell.Add(CellId, Written._ActorNamesByGroupIndex);
        ++Stats._NumCellsWritten;
    }

    // ---- Rebuild the index -------------------------------------------------------------------------

    const auto Remap = ComputeIndexRemap(RemapInput);

    auto NewCellRefs = TArray<FCk_Jolt_CookedCellRef>{};
    NewCellRefs.SetNum(Remap._NumNewCells);

    for (auto CellIndex = 0; CellIndex < Cells.Num(); ++CellIndex)
    {
        const auto NewCellIndex = Remap._NewCellIndexByOldCellIndex[CellIndex];

        if (NewCellIndex == INDEX_NONE)
        { continue; }

        NewCellRefs[NewCellIndex] = Cells[CellIndex];
    }

    for (const auto& [WrittenCellId, NewCellIndex] : Remap._NewCellIndexByWrittenCellId)
    { NewCellRefs[NewCellIndex] = FreshCellRefs[WrittenCellId]; }

    Stats._NumCells = NewCellRefs.Num();
    Stats._NumUniqueShapes = ShapeCache.Get_NumUniqueShapes();

    auto NewActorLookup = Remap._ActorLookup;

    auto IndexPackageName = FString{};
    if (NOT DoWrite_Index(RootPath, MapSubPath, MapPackageName, BakeFilter,
        MoveTemp(NewCellRefs), MoveTemp(NewActorLookup), IndexPackageName))
    { return Stats; }

    ck::jolt::Log(TEXT("JoltCook incremental: map [{}] — [{}] changed, [{}] added, [{}] removed, [{}] unchanged, "
        "[{}] preserved in unloaded levels -> [{}] of [{}] cells rewritten -> [{}]"),
        MapPackageName, Plan._NumChangedActors, Plan._NumAddedActors, Plan._RemovedActorNames.Num(),
        Plan._NumUnchangedActors, Plan._NumPreservedUnloadedActors, Stats._NumCellsWritten, Cells.Num(),
        IndexPackageName);

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
            if (ck::Is_NOT_Valid(CellAsset))
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
