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
#include <Math/NumericLimits.h>
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
        const TSet<FName>& InPresentActorNames,
        const TSet<FName>& InLoadedLevelPackages,
        TArray<FActorCookData>& OutActors)
        -> TOptional<int32>
    {
        const auto Get_NeedsCarryOver = [&](const FCk_Jolt_CookedActorGroup& InGroup)
        {
            return NOT InPresentActorNames.Contains(InGroup.Get_SourceActorName())
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

struct FCk_Jolt_IncrementalCookDriver::FImpl
{
    enum class EPhase : uint8
    {
        Prepare,
        LoadCells,
        Sweep,
        Plan,
        WriteCells,
        WriteIndex,
        Complete,
        Failed,
        FullCookRequired
    };

    TWeakObjectPtr<UWorld> _World;
    ck::jolt::cook::ECk_Jolt_CookMode _Mode = ck::jolt::cook::ECk_Jolt_CookMode::Cook;
    EPhase _Phase = EPhase::Prepare;

    FString _RootPath;
    FString _MapPackageName;
    FString _MapSubPath;
    float _CellSize = 0.0f;
    ck::jolt::bake::FCk_Jolt_BakeFilter _BakeFilter;

    TStrongObjectPtr<UCk_Jolt_CookedWorldIndex_UE> _Index;
    TArray<TStrongObjectPtr<UCk_Jolt_CookedCell_UE>> _CookedCells;
    TArray<FIntPoint> _CookedCellIds;
    TMap<FIntPoint, int32> _CookedCellIdToIndex;

    TArray<TWeakObjectPtr<AActor>> _ActorsToSweep;
    TMap<FName, TWeakObjectPtr<AActor>> _PresentActors;
    TSet<FName> _PresentActorNames;
    TMap<FName, const ck::jolt::cook::FCk_Jolt_IncrementalCookedActor*> _CookedByName;

    ck::jolt::cook::FCk_Jolt_IncrementalPlanInput _PlanInput;
    ck::jolt::cook::FCk_Jolt_IncrementalPlan _Plan;
    TArray<FIntPoint> _DirtyCellIds;
    TMap<FIntPoint, TArray<FName>> _PresentActorNamesByDirtyCell;

    ck::jolt::bake::FCk_Jolt_ShapeCache _ShapeCache;
    ck::jolt::cook::FCk_Jolt_IndexRemapInput _RemapInput;
    TMap<FIntPoint, FCk_Jolt_CookedCellRef> _FreshCellRefs;

    FCk_Jolt_WorldCooker::FCookStats _Stats;

    int32 _Cursor = 0;
    int32 _CompletedUnits = 0;

    auto Get_TotalUnits() const -> int32
    {
        return _CookedCellIds.Num() + _ActorsToSweep.Num() + _DirtyCellIds.Num() + 1;
    }

    auto DoDecline(ck::jolt::cook::ECk_Jolt_IncrementalOutcome InOutcome, const FString& InReason)
        -> ck::jolt::cook::ECk_Jolt_CookStepResult
    {
        ck::jolt::Log(TEXT("JoltCook: incremental cook of [{}] declined ({}) — a FULL cook is required"),
            _MapPackageName, InReason);

        _Stats._Outcome = InOutcome;
        _Phase = EPhase::FullCookRequired;
        return ck::jolt::cook::ECk_Jolt_CookStepResult::FullCookRequired;
    }

    auto DoStep_Prepare() -> ck::jolt::cook::ECk_Jolt_CookStepResult;
    auto DoStep_LoadCells(FCk_Time InBudget) -> ck::jolt::cook::ECk_Jolt_CookStepResult;
    auto DoStep_Sweep(FCk_Time InBudget) -> ck::jolt::cook::ECk_Jolt_CookStepResult;
    auto DoStep_Plan() -> ck::jolt::cook::ECk_Jolt_CookStepResult;
    auto DoStep_WriteCells(FCk_Time InBudget) -> ck::jolt::cook::ECk_Jolt_CookStepResult;
    auto DoStep_WriteIndex() -> ck::jolt::cook::ECk_Jolt_CookStepResult;
};

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_IncrementalCookDriver::FImpl::
    DoStep_Prepare()
    -> ck::jolt::cook::ECk_Jolt_CookStepResult
{
    using namespace ck_jolt_cook_world_cooker;
    using namespace ck::jolt;
    using namespace ck::jolt::bake;
    using namespace ck::jolt::cook;

    auto* World = _World.Get();

    const auto WorldIsValid = ck::IsValid(World);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("Incremental Jolt cook lost its world"))
    {
        _Phase = EPhase::Failed;
        return ECk_Jolt_CookStepResult::Failed;
    }

    _RootPath = UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath();
    _MapPackageName = World->PersistentLevel->GetOutermost()->GetName();
    _MapSubPath = Get_MapSubPath(_MapPackageName);
    _CellSize = UCk_Utils_Jolt_ProjectSettings::Get_BakeGridCellSize();
    _BakeFilter = FCk_Jolt_BakeFilter::Make_FromProjectSettings();

    // ForEachActorWithLoading releases each actor once its batch is done, so the actors sharing a
    // dirty cell with a changed one cannot be revisited.
    if (World->GetWorldPartition() != nullptr)
    { return DoDecline(ECk_Jolt_IncrementalOutcome::FullCook_WorldPartition, TEXT("World Partition world")); }

    const auto IndexPath = Get_CookedIndexAssetPath(_RootPath, _MapPackageName);
    _Index.Reset(LoadObject<UCk_Jolt_CookedWorldIndex_UE>(nullptr, *IndexPath, nullptr,
        LOAD_NoWarn | LOAD_Quiet));

    if (ck::Is_NOT_Valid(_Index.Get()))
    { return DoDecline(ECk_Jolt_IncrementalOutcome::FullCook_NoExistingIndex, TEXT("map was never cooked")); }

    const auto VersionsMatch = _Index->Get_CookVersion() == CookVersion_Current
        && _Index->Get_JoltVersionId() == static_cast<uint32>(JPH_VERSION_ID);

    if (NOT VersionsMatch)
    { return DoDecline(ECk_Jolt_IncrementalOutcome::FullCook_ContractDrift, TEXT("cook/Jolt version drift")); }

    if (_Index->Get_BakeFilterHash() != _BakeFilter.ComputeHash())
    { return DoDecline(ECk_Jolt_IncrementalOutcome::FullCook_ContractDrift, TEXT("bake-filter settings changed")); }

    _CookedCellIds = ck::algo::Transform<TArray<FIntPoint>>(_Index->Get_Cells(),
        [](const FCk_Jolt_CookedCellRef& InCellRef) { return InCellRef.Get_CellId(); });

    for (auto CellIndex = 0; CellIndex < _CookedCellIds.Num(); ++CellIndex)
    { _CookedCellIdToIndex.Add(_CookedCellIds[CellIndex], CellIndex); }

    _PlanInput._LoadedLevelPackages = Get_LoadedLevelPackages(*World);

    for (const auto& Level : World->GetLevels())
    {
        if (ck::Is_NOT_Valid(Level))
        { continue; }

        for (const auto& Actor : Level->Actors)
        {
            if (ck::Is_NOT_Valid(Actor))
            { continue; }

            _ActorsToSweep.Emplace(Actor);
        }
    }

    _Cursor = 0;
    _Phase = EPhase::LoadCells;
    return ECk_Jolt_CookStepResult::InProgress;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_IncrementalCookDriver::FImpl::
    DoStep_LoadCells(
        FCk_Time InBudget)
    -> ck::jolt::cook::ECk_Jolt_CookStepResult
{
    using namespace ck_jolt_cook_world_cooker;
    using namespace ck::jolt;
    using namespace ck::jolt::bake;
    using namespace ck::jolt::cook;

    const auto SliceStart = FPlatformTime::Seconds();

    while (_Cursor < _CookedCellIds.Num())
    {
        const auto& CellRef = _Index->Get_Cells()[_Cursor];
        auto* CellAsset = CellRef.Get_CellAsset().LoadSynchronous();

        if (ck::Is_NOT_Valid(CellAsset))
        {
            return DoDecline(ECk_Jolt_IncrementalOutcome::FullCook_ContractDrift,
                ck::Format_UE(TEXT("cell [{}] failed to load"), CellRef.Get_CellId()));
        }

        _CookedCells.Emplace(TStrongObjectPtr<UCk_Jolt_CookedCell_UE>{CellAsset});

        for (const auto& Group : CellAsset->Get_ActorGroups())
        {
            auto CookedActor = FCk_Jolt_IncrementalCookedActor{};
            CookedActor._ActorName = Group.Get_SourceActorName();
            CookedActor._SourceHash = Group.Get_SourceHash();
            CookedActor._CellId = CellRef.Get_CellId();
            CookedActor._OwningLevelPackage = Get_LevelPackageOfCookedActor(Group);

            _PlanInput._Cooked.Emplace(MoveTemp(CookedActor));
        }

        ++_Cursor;
        ++_CompletedUnits;

        if (FCk_Time{FPlatformTime::Seconds() - SliceStart} >= InBudget)
        { break; }
    }

    if (_Cursor < _CookedCellIds.Num())
    { return ECk_Jolt_CookStepResult::InProgress; }

    // Pointers into _PlanInput._Cooked, which is complete and never resized again.
    ck::algo::ForEach(_PlanInput._Cooked, [&](const FCk_Jolt_IncrementalCookedActor& InCooked)
    {
        _CookedByName.Add(InCooked._ActorName, &InCooked);
    });

    _Cursor = 0;
    _Phase = EPhase::Sweep;
    return ECk_Jolt_CookStepResult::InProgress;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_IncrementalCookDriver::FImpl::
    DoStep_Sweep(
        FCk_Time InBudget)
    -> ck::jolt::cook::ECk_Jolt_CookStepResult
{
    using namespace ck_jolt_cook_world_cooker;
    using namespace ck::jolt;
    using namespace ck::jolt::bake;
    using namespace ck::jolt::cook;

    const auto SliceStart = FPlatformTime::Seconds();

    while (_Cursor < _ActorsToSweep.Num())
    {
        auto* Actor = _ActorsToSweep[_Cursor].Get();
        ++_Cursor;
        ++_CompletedUnits;

        if (ck::Is_NOT_Valid(Actor))
        { continue; }

        const auto ActorName = Actor->GetFName();
        const auto SourceHash = ComputeSourceHash(*Actor, _BakeFilter);

        auto Present = FCk_Jolt_IncrementalPresentActor{};
        Present._ActorName = ActorName;
        Present._SourceHash = SourceHash;

        const auto* const* CookedActor = _CookedByName.Find(ActorName);
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
            DoExtract_Actor(*Actor, _ShapeCache, _BakeFilter, Extracted);

            Present._HasBodies = Extracted.Num() > 0;

            if (Present._HasBodies)
            { Present._CurrentCellId = Get_CellIdForActor(Extracted[0], _CellSize); }
        }

        if (Present._HasBodies)
        { ++_Stats._NumActors; }

        _PresentActors.Add(ActorName, Actor);
        _PresentActorNames.Add(ActorName);
        _PlanInput._Present.Emplace(MoveTemp(Present));

        if (FCk_Time{FPlatformTime::Seconds() - SliceStart} >= InBudget)
        { break; }
    }

    if (_Cursor < _ActorsToSweep.Num())
    { return ECk_Jolt_CookStepResult::InProgress; }

    _Phase = EPhase::Plan;
    return ECk_Jolt_CookStepResult::InProgress;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_IncrementalCookDriver::FImpl::
    DoStep_Plan()
    -> ck::jolt::cook::ECk_Jolt_CookStepResult
{
    using namespace ck_jolt_cook_world_cooker;
    using namespace ck::jolt;
    using namespace ck::jolt::bake;
    using namespace ck::jolt::cook;

    _Plan = ComputeIncrementalPlan(_PlanInput);

    _Stats._NumActorsUpToDate = _Plan._NumUnchangedActors;
    _Stats._NumCells = _CookedCellIds.Num();
    _Stats._Outcome = ECk_Jolt_IncrementalOutcome::Incremental;

    if (_Plan._DirtyCellIds.IsEmpty())
    {
        ck::jolt::Log(TEXT("JoltCook incremental: map [{}] is up to date — [{}] actors checked, [{}] preserved "
            "in unloaded levels, nothing rewritten"),
            _MapPackageName, _Plan._NumUnchangedActors, _Plan._NumPreservedUnloadedActors);

        _Stats._Success = true;
        _Phase = EPhase::Complete;
        return ECk_Jolt_CookStepResult::Done;
    }

    if (_Mode == ECk_Jolt_CookMode::DryRun)
    {
        ck::jolt::Log(TEXT("JoltCook incremental DRY RUN: map [{}] — [{}] changed, [{}] added, [{}] removed, "
            "[{}] unchanged, [{}] preserved in unloaded levels -> [{}] of [{}] cells would be rewritten"),
            _MapPackageName, _Plan._NumChangedActors, _Plan._NumAddedActors, _Plan._RemovedActorNames.Num(),
            _Plan._NumUnchangedActors, _Plan._NumPreservedUnloadedActors, _Plan._DirtyCellIds.Num(),
            _CookedCellIds.Num());

        _Stats._Success = true;
        _Phase = EPhase::Complete;
        return ECk_Jolt_CookStepResult::Done;
    }

    _DirtyCellIds = _Plan._DirtyCellIds.Array();

    for (const auto& Present : _PlanInput._Present)
    {
        if (NOT Present._HasBodies || NOT _Plan._DirtyCellIds.Contains(Present._CurrentCellId))
        { continue; }

        _PresentActorNamesByDirtyCell.FindOrAdd(Present._CurrentCellId).Emplace(Present._ActorName);
    }

    _RemapInput._DirtyCellIds = _Plan._DirtyCellIds;
    _RemapInput._ExistingActorLookup = _Index->Get_ActorLookup();
    _RemapInput._ExistingCellIdsByCellIndex = _CookedCellIds;

    _Cursor = 0;
    _Phase = EPhase::WriteCells;
    return ECk_Jolt_CookStepResult::InProgress;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_IncrementalCookDriver::FImpl::
    DoStep_WriteCells(
        FCk_Time InBudget)
    -> ck::jolt::cook::ECk_Jolt_CookStepResult
{
    using namespace ck_jolt_cook_world_cooker;
    using namespace ck::jolt;
    using namespace ck::jolt::bake;
    using namespace ck::jolt::cook;

    const auto SliceStart = FPlatformTime::Seconds();

    while (_Cursor < _DirtyCellIds.Num())
    {
        const auto DirtyCellId = _DirtyCellIds[_Cursor];
        ++_Cursor;
        ++_CompletedUnits;

        auto Cell = FCellCookData{};
        Cell._CellId = DirtyCellId;

        if (const auto* ActorNames = _PresentActorNamesByDirtyCell.Find(DirtyCellId))
        {
            for (const auto& ActorName : *ActorNames)
            {
                auto* Actor = _PresentActors.FindRef(ActorName).Get();

                if (ck::Is_NOT_Valid(Actor))
                { continue; }

                _Stats._NumBodies += DoExtract_Actor(*Actor, _ShapeCache, _BakeFilter, Cell._Actors);
            }
        }

        if (const auto* CookedCellIndex = _CookedCellIdToIndex.Find(DirtyCellId))
        {
            const auto CarriedOver = DoCarryOver_ActorsInUnloadedLevels(
                *_CookedCells[*CookedCellIndex].Get(), _PresentActorNames, _PlanInput._LoadedLevelPackages,
                Cell._Actors);

            if (NOT CarriedOver.IsSet())
            {
                _Phase = EPhase::Failed;
                return ECk_Jolt_CookStepResult::Failed;
            }

            _Stats._NumBodies += CarriedOver.GetValue();
        }

        if (Cell._Actors.IsEmpty())
        {
            ck::jolt::Warning(TEXT("JoltCook incremental: cell [{}] of map [{}] no longer holds any baked actor "
                "— it is dropped from the index and its cooked asset is now ORPHANED; delete it by hand"),
                DirtyCellId, _MapPackageName);
            continue;
        }

        const auto Written = DoWrite_Cell(Cell, _RootPath, _MapSubPath);

        if (NOT Written._Success)
        {
            _Phase = EPhase::Failed;
            return ECk_Jolt_CookStepResult::Failed;
        }

        _FreshCellRefs.Add(DirtyCellId, Written._CellRef);
        _RemapInput._WrittenCellIds.Emplace(DirtyCellId);
        _RemapInput._WrittenActorNamesByCell.Add(DirtyCellId, Written._ActorNamesByGroupIndex);
        ++_Stats._NumCellsWritten;

        if (FCk_Time{FPlatformTime::Seconds() - SliceStart} >= InBudget)
        { break; }
    }

    if (_Cursor < _DirtyCellIds.Num())
    { return ECk_Jolt_CookStepResult::InProgress; }

    _Phase = EPhase::WriteIndex;
    return ECk_Jolt_CookStepResult::InProgress;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_IncrementalCookDriver::FImpl::
    DoStep_WriteIndex()
    -> ck::jolt::cook::ECk_Jolt_CookStepResult
{
    using namespace ck_jolt_cook_world_cooker;
    using namespace ck::jolt;
    using namespace ck::jolt::bake;
    using namespace ck::jolt::cook;

    const auto Remap = ComputeIndexRemap(_RemapInput);

    auto NewCellRefs = TArray<FCk_Jolt_CookedCellRef>{};
    NewCellRefs.SetNum(Remap._NumNewCells);

    for (auto CellIndex = 0; CellIndex < _CookedCellIds.Num(); ++CellIndex)
    {
        const auto NewCellIndex = Remap._NewCellIndexByOldCellIndex[CellIndex];

        if (NewCellIndex == INDEX_NONE)
        { continue; }

        NewCellRefs[NewCellIndex] = _Index->Get_Cells()[CellIndex];
    }

    for (const auto& [WrittenCellId, NewCellIndex] : Remap._NewCellIndexByWrittenCellId)
    { NewCellRefs[NewCellIndex] = _FreshCellRefs[WrittenCellId]; }

    _Stats._NumCells = NewCellRefs.Num();
    _Stats._NumUniqueShapes = _ShapeCache.Get_NumUniqueShapes();

    auto NewActorLookup = Remap._ActorLookup;
    auto IndexPackageName = FString{};

    if (NOT DoWrite_Index(_RootPath, _MapSubPath, _MapPackageName, _BakeFilter,
        MoveTemp(NewCellRefs), MoveTemp(NewActorLookup), IndexPackageName))
    {
        _Phase = EPhase::Failed;
        return ECk_Jolt_CookStepResult::Failed;
    }

    ++_CompletedUnits;

    ck::jolt::Log(TEXT("JoltCook incremental: map [{}] — [{}] changed, [{}] added, [{}] removed, [{}] unchanged, "
        "[{}] preserved in unloaded levels -> [{}] of [{}] cells rewritten -> [{}]"),
        _MapPackageName, _Plan._NumChangedActors, _Plan._NumAddedActors, _Plan._RemovedActorNames.Num(),
        _Plan._NumUnchangedActors, _Plan._NumPreservedUnloadedActors, _Stats._NumCellsWritten,
        _CookedCellIds.Num(), IndexPackageName);

    _Stats._Success = true;
    _Phase = EPhase::Complete;
    return ECk_Jolt_CookStepResult::Done;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Jolt_IncrementalCookDriver::
    FCk_Jolt_IncrementalCookDriver(
        UWorld& InWorld,
        ck::jolt::cook::ECk_Jolt_CookMode InMode)
    : _Impl{MakeUnique<FImpl>()}
{
    ck::jolt::Request_GlobalJoltInit();

    _Impl->_World = &InWorld;
    _Impl->_Mode = InMode;
}

FCk_Jolt_IncrementalCookDriver::
    ~FCk_Jolt_IncrementalCookDriver()
{
    // The impl holds JPH::Refs; they must die before the factory that made them.
    _Impl.Reset();

    ck::jolt::Request_GlobalJoltShutdown();
}

auto
    FCk_Jolt_IncrementalCookDriver::
    Step(
        FCk_Time InBudget)
    -> ck::jolt::cook::ECk_Jolt_CookStepResult
{
    using namespace ck::jolt::cook;
    using EPhase = FImpl::EPhase;

    switch (_Impl->_Phase)
    {
        case EPhase::Prepare:          return _Impl->DoStep_Prepare();
        case EPhase::LoadCells:        return _Impl->DoStep_LoadCells(InBudget);
        case EPhase::Sweep:            return _Impl->DoStep_Sweep(InBudget);
        case EPhase::Plan:             return _Impl->DoStep_Plan();
        case EPhase::WriteCells:       return _Impl->DoStep_WriteCells(InBudget);
        case EPhase::WriteIndex:       return _Impl->DoStep_WriteIndex();
        case EPhase::Complete:         return ECk_Jolt_CookStepResult::Done;
        case EPhase::Failed:           return ECk_Jolt_CookStepResult::Failed;
        case EPhase::FullCookRequired: return ECk_Jolt_CookStepResult::FullCookRequired;
    }

    return ECk_Jolt_CookStepResult::Failed;
}

auto
    FCk_Jolt_IncrementalCookDriver::
    Get_CompletedUnits() const
    -> int32
{
    return _Impl->_CompletedUnits;
}

auto
    FCk_Jolt_IncrementalCookDriver::
    Get_TotalUnits() const
    -> int32
{
    return _Impl->Get_TotalUnits();
}

auto
    FCk_Jolt_IncrementalCookDriver::
    Get_Stats() const
    -> FCk_Jolt_WorldCooker::FCookStats
{
    return _Impl->_Stats;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_WorldCooker::
    Cook_World_Incremental(
        UWorld& InWorld,
        ck::jolt::cook::ECk_Jolt_CookMode InMode)
    -> FCookStats
{
    using namespace ck::jolt::cook;

    auto Driver = FCk_Jolt_IncrementalCookDriver{InWorld, InMode};

    // Synchronous callers (commandlet, editor-utility Blueprints) want it finished on return.
    constexpr auto NoBudget = FCk_Time{TNumericLimits<double>::Max()};

    auto Result = ECk_Jolt_CookStepResult::InProgress;
    while (Result == ECk_Jolt_CookStepResult::InProgress)
    { Result = Driver.Step(NoBudget); }

    if (Result == ECk_Jolt_CookStepResult::FullCookRequired)
    {
        auto FullStats = Cook_World(InWorld, InMode);
        FullStats._Outcome = Driver.Get_Stats()._Outcome;
        return FullStats;
    }

    return Driver.Get_Stats();
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
