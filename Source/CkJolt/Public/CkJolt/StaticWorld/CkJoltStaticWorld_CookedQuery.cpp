#include "CkJolt/StaticWorld/CkJoltStaticWorld_CookedQuery.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/CollisionLayers/CkJoltCollisionLayerTable.h"
#include "CkJolt/Query/CkJoltOccupancy_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Data.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include <sstream>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_static_world_cooked_query
{
    using namespace ck::jolt;

    auto
    Get_IsFiniteVector(
        const FVector& InValue) -> bool
    {
        return FMath::IsFinite(InValue.X) && FMath::IsFinite(InValue.Y) && FMath::IsFinite(InValue.Z);
    }

    auto
    Get_IsUsableBoxQuery(
        const FVector& InCenter,
        const FVector& InHalfExtents) -> bool
    {
        return Get_IsFiniteVector(InCenter) && Get_IsFiniteVector(InHalfExtents) &&
               InHalfExtents.X > 0.0f && InHalfExtents.Y > 0.0f && InHalfExtents.Z > 0.0f;
    }

    auto
    Make_Result(
        ECk_Jolt_CookedWorldQueryLoadStatus InStatus,
        FString InMessage) -> FCk_Jolt_CookedWorldQueryLoadResult
    {
        auto Result = FCk_Jolt_CookedWorldQueryLoadResult{};
        Result._Status = InStatus;
        Result._Message = MoveTemp(InMessage);
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    struct FCk_Jolt_CookedWorldQuery::FImpl
    {
        FImpl()
        {
            Request_GlobalJoltInit();

            _LayerTable.Build_FromCollisionProfiles();
            _BroadPhaseLayerInterface = MakeUnique<FCk_Jolt_BroadPhaseLayerInterface_Table>(_LayerTable);
            _ObjectVsBroadPhaseFilter = MakeUnique<FCk_Jolt_ObjectVsBroadPhaseLayerFilter_Table>(_LayerTable);
            _ObjectVsObjectFilter = MakeUnique<FCk_Jolt_ObjectLayerPairFilter_Table>(_LayerTable);
            _PhysicsSystem = MakeUnique<JPH::PhysicsSystem>();

            _PhysicsSystem->Init(
                static_cast<uint32>(UCk_Utils_Jolt_ProjectSettings::Get_MaxBodies()), 0,
                static_cast<uint32>(UCk_Utils_Jolt_ProjectSettings::Get_MaxBodyPairs()),
                static_cast<uint32>(UCk_Utils_Jolt_ProjectSettings::Get_MaxContactConstraints()),
                *_BroadPhaseLayerInterface, *_ObjectVsBroadPhaseFilter, *_ObjectVsObjectFilter);
        }

        ~FImpl()
        {
            _PhysicsSystem.Reset();
            _ObjectVsObjectFilter.Reset();
            _ObjectVsBroadPhaseFilter.Reset();
            _BroadPhaseLayerInterface.Reset();
            Request_GlobalJoltShutdown();
        }

        FCk_Jolt_CollisionLayerTable _LayerTable;
        TUniquePtr<FCk_Jolt_BroadPhaseLayerInterface_Table> _BroadPhaseLayerInterface;
        TUniquePtr<FCk_Jolt_ObjectVsBroadPhaseLayerFilter_Table> _ObjectVsBroadPhaseFilter;
        TUniquePtr<FCk_Jolt_ObjectLayerPairFilter_Table> _ObjectVsObjectFilter;
        TUniquePtr<JPH::PhysicsSystem> _PhysicsSystem;
        FBox _LoadedBounds = FBox{ForceInit};
        FCk_Jolt_CookedWorldQueryLoadResult _Result;
    };

    // ----------------------------------------------------------------------------------------------------------------

    FCk_Jolt_CookedWorldQuery::
        FCk_Jolt_CookedWorldQuery()
        : _Impl(MakeUnique<FImpl>())
    {}

    FCk_Jolt_CookedWorldQuery::
        ~FCk_Jolt_CookedWorldQuery() = default;

    auto
        FCk_Jolt_CookedWorldQuery::
        Request_Load(const FCk_Jolt_CookedWorldQueryLoadRequest& InRequest) -> FCk_Jolt_CookedWorldQueryLoadResult
    {
        using namespace ck_jolt_static_world_cooked_query;

        // A failed load may already have restored bodies before discovering a later corrupt cell. Recreate the
        // private world rather than trying to subtract a partial population, so a subsequent successful load
        // cannot query geometry inherited from the rejected one.
        _Impl = MakeUnique<FImpl>();

        const auto RequestIsValid = NOT InRequest._CookedDataRootPath.IsEmpty() && NOT InRequest._MapPackageName.IsEmpty() &&
                                    (InRequest._OptionalBounds.IsValid == 0 ||
                                     (Get_IsFiniteVector(InRequest._OptionalBounds.Min) &&
                                      Get_IsFiniteVector(InRequest._OptionalBounds.Max)));

        CK_ENSURE_IF_NOT(RequestIsValid,
            TEXT("Cooked Jolt query needs a non-empty root/map and finite optional bounds"))
        {}

        if (NOT RequestIsValid)
        {
            _Impl->_Result = Make_Result(ECk_Jolt_CookedWorldQueryLoadStatus::InvalidRequest,
                TEXT("Cooked Jolt query requires a non-empty root/map and finite optional bounds"));
            return _Impl->_Result;
        }

        const auto IndexPath = Get_CookedIndexAssetPath(InRequest._CookedDataRootPath, InRequest._MapPackageName);
        const auto* Index = LoadObject<UCk_Jolt_CookedWorldIndex_UE>(nullptr, *IndexPath);

        const auto IndexLoaded = ck::IsValid(Index, ck::IsValid_Policy_NullptrOnly{});

        CK_ENSURE_IF_NOT(IndexLoaded, TEXT("Cooked Jolt query found no index at [{}]"), IndexPath)
        {}

        if (NOT IndexLoaded)
        {
            _Impl->_Result = Make_Result(ECk_Jolt_CookedWorldQueryLoadStatus::MissingIndex,
                FString::Printf(TEXT("No cooked Jolt index at [%s]"), *IndexPath));
            return _Impl->_Result;
        }

        const auto IndexMatchesMap = Index->Get_SourceMapPackage() == FName{*InRequest._MapPackageName};
        const auto IndexMatchesVersion = Index->Get_CookVersion() == CookVersion_Current &&
                                         Index->Get_JoltVersionId() == static_cast<uint32>(JPH_VERSION_ID);
        const auto IndexIsCurrent = IndexMatchesMap && IndexMatchesVersion;

        CK_ENSURE_IF_NOT(IndexIsCurrent,
            TEXT("Cooked Jolt index [{}] is stale or names a different map"), IndexPath)
        {}

        if (NOT IndexIsCurrent)
        {
            _Impl->_Result = Make_Result(ECk_Jolt_CookedWorldQueryLoadStatus::StaleIndex,
                FString::Printf(TEXT("Cooked Jolt index [%s] is stale or names a different map"), *IndexPath));
            return _Impl->_Result;
        }

        auto* BodyInterface = &_Impl->_PhysicsSystem->GetBodyInterface();
        auto LoadedCellCount = 0;
        auto LoadedBodyCount = 0;

        for (const auto& CellRef : Index->Get_Cells())
        {
            if (InRequest._OptionalBounds.IsValid != 0 && NOT CellRef.Get_Bounds().Intersect(InRequest._OptionalBounds))
            { continue; }

            const auto* CellAsset = CellRef.Get_CellAsset().LoadSynchronous();
            const auto CellLoaded = ck::IsValid(CellAsset, ck::IsValid_Policy_NullptrOnly{});

            CK_ENSURE_IF_NOT(CellLoaded, TEXT("Cooked Jolt query cell [{}] failed to load"), CellRef.Get_CellId())
            {}

            if (NOT CellLoaded)
            {
                _Impl->_Result = Make_Result(ECk_Jolt_CookedWorldQueryLoadStatus::MissingCell,
                    FString::Printf(TEXT("Cooked Jolt cell [%d,%d] failed to load"), CellRef.Get_CellId().X, CellRef.Get_CellId().Y));
                return _Impl->_Result;
            }

            const auto CellMatchesVersion = CellAsset->Get_CookVersion() == CookVersion_Current &&
                                            CellAsset->Get_JoltVersionId() == static_cast<uint32>(JPH_VERSION_ID);

            CK_ENSURE_IF_NOT(CellMatchesVersion, TEXT("Cooked Jolt query cell [{}] is stale"), CellRef.Get_CellId())
            {}

            if (NOT CellMatchesVersion)
            {
                _Impl->_Result = Make_Result(ECk_Jolt_CookedWorldQueryLoadStatus::StaleCell,
                    FString::Printf(TEXT("Cooked Jolt cell [%d,%d] is stale"), CellRef.Get_CellId().X, CellRef.Get_CellId().Y));
                return _Impl->_Result;
            }

            const auto& Blob = CellAsset->Get_ShapeBlob();
            auto BlobStream = std::istringstream{
                std::string{reinterpret_cast<const char*>(Blob.GetData()), static_cast<size_t>(Blob.Num())}};
            auto StreamWrapper = JPH::StreamInWrapper{BlobStream};
            auto IdToShape = JPH::Shape::IDToShapeMap{};
            auto IdToMaterial = JPH::Shape::IDToMaterialMap{};
            auto Shapes = TArray<JPH::Ref<JPH::Shape>>{};
            Shapes.Reserve(CellAsset->Get_ShapeCount());

            for (auto ShapeIndex = 0; ShapeIndex < CellAsset->Get_ShapeCount(); ++ShapeIndex)
            {
                const auto RestoredShape = JPH::Shape::sRestoreWithChildren(StreamWrapper, IdToShape, IdToMaterial);
                const auto ShapeRestored = RestoredShape.IsValid();

                CK_ENSURE_IF_NOT(ShapeRestored, TEXT("Cooked Jolt query cell [{}] shape [{}] failed to restore"),
                    CellRef.Get_CellId(), ShapeIndex)
                {}

                if (NOT ShapeRestored)
                {
                    _Impl->_Result = Make_Result(ECk_Jolt_CookedWorldQueryLoadStatus::CorruptCell,
                        FString::Printf(TEXT("Cooked Jolt cell [%d,%d] has a corrupt shape blob"), CellRef.Get_CellId().X, CellRef.Get_CellId().Y));
                    return _Impl->_Result;
                }

                Shapes.Emplace(RestoredShape.Get());
            }

            for (const auto& Group : CellAsset->Get_ActorGroups())
            {
                const auto* CurrentRuntimeHash = InRequest._CurrentActorRuntimeHashes.Find(Group.Get_SourceActorName());
                const auto ActorHashMatches = CurrentRuntimeHash != nullptr && *CurrentRuntimeHash == Group.Get_RuntimeCheckHash();
                const auto ActorHashIsAccepted = NOT InRequest._RequireCurrentActorRuntimeHashes || ActorHashMatches;

                CK_ENSURE_IF_NOT(ActorHashIsAccepted,
                    TEXT("Cooked Jolt query actor [{}] does not match its cooked runtime hash"), Group.Get_SourceActorName())
                {}

                if (NOT ActorHashIsAccepted)
                {
                    _Impl->_Result = Make_Result(ECk_Jolt_CookedWorldQueryLoadStatus::StaleActor,
                        FString::Printf(TEXT("Cooked Jolt actor [%s] is missing or stale"), *Group.Get_SourceActorName().ToString()));
                    return _Impl->_Result;
                }

                for (const auto& Record : Group.Get_Bodies())
                {
                    const auto ShapeIndexIsValid = Shapes.IsValidIndex(Record.Get_ShapeIndex());

                    CK_ENSURE_IF_NOT(ShapeIndexIsValid,
                        TEXT("Cooked Jolt query cell [{}] actor [{}] references shape [{}] outside [{}]"),
                        CellRef.Get_CellId(), Group.Get_SourceActorName(), Record.Get_ShapeIndex(), Shapes.Num())
                    {}

                    if (NOT ShapeIndexIsValid)
                    {
                        _Impl->_Result = Make_Result(ECk_Jolt_CookedWorldQueryLoadStatus::CorruptCell,
                            FString::Printf(TEXT("Cooked Jolt cell [%d,%d] references an invalid shape"), CellRef.Get_CellId().X, CellRef.Get_CellId().Y));
                        return _Impl->_Result;
                    }

                    const auto Layer = _Impl->_LayerTable.Get_OrRegisterLayer(Record.Get_Signature());
                    const auto LayerIsValid = Layer != JPH::cObjectLayerInvalid;

                    CK_ENSURE_IF_NOT(LayerIsValid, TEXT("Cooked Jolt query ran out of collision layers"))
                    {}

                    if (NOT LayerIsValid)
                    {
                        _Impl->_Result = Make_Result(ECk_Jolt_CookedWorldQueryLoadStatus::CorruptCell,
                            TEXT("Cooked Jolt query could not register a collision layer"));
                        return _Impl->_Result;
                    }

                    auto BodySettings = JPH::BodyCreationSettings{
                        Shapes[Record.Get_ShapeIndex()].GetPtr(), Conv(Record.Get_Position()), Conv(Record.Get_Rotation()),
                        JPH::EMotionType::Static, JPH::ObjectLayer{Layer}};
                    BodySettings.mFriction = Record.Get_Friction();
                    BodySettings.mRestitution = Record.Get_Restitution();

                    const auto BodyId = BodyInterface->CreateAndAddBody(BodySettings, JPH::EActivation::DontActivate);
                    const auto BodyCreated = !BodyId.IsInvalid();

                    CK_ENSURE_IF_NOT(BodyCreated, TEXT("Cooked Jolt query exhausted static body capacity"))
                    {}

                    if (NOT BodyCreated)
                    {
                        _Impl->_Result = Make_Result(ECk_Jolt_CookedWorldQueryLoadStatus::BodyCapacityExceeded,
                            TEXT("Cooked Jolt query exhausted static body capacity"));
                        return _Impl->_Result;
                    }

                    ++LoadedBodyCount;
                }
            }

            _Impl->_LoadedBounds += CellRef.Get_Bounds();
            ++LoadedCellCount;
        }

        _Impl->_Result = Make_Result(ECk_Jolt_CookedWorldQueryLoadStatus::Ready,
            FString::Printf(TEXT("Loaded %d cooked Jolt cells and %d bodies"), LoadedCellCount, LoadedBodyCount));
        _Impl->_Result._LoadedCellCount = LoadedCellCount;
        _Impl->_Result._LoadedBodyCount = LoadedBodyCount;
        return _Impl->_Result;
    }

    auto
        FCk_Jolt_CookedWorldQuery::
        Get_LoadResult() const -> const FCk_Jolt_CookedWorldQueryLoadResult&
    {
        return _Impl->_Result;
    }

    auto
        FCk_Jolt_CookedWorldQuery::
        Get_IsReady() const -> bool
    {
        return _Impl->_Result.Get_IsReady();
    }

    auto
        FCk_Jolt_CookedWorldQuery::
        Get_LoadedBounds() const -> const FBox&
    {
        return _Impl->_LoadedBounds;
    }

    auto
        FCk_Jolt_CookedWorldQuery::
        Get_IsBoxOccupied(
            const FVector& InCenter,
            const FVector& InHalfExtents) const -> bool
    {
        using namespace ck_jolt_static_world_cooked_query;

        if (NOT Get_IsReady() || NOT Get_IsUsableBoxQuery(InCenter, InHalfExtents))
        { return false; }

        const auto BroadPhaseFilter = FCk_Jolt_StaticBroadPhaseQueryFilter{};
        const auto ObjectFilter = FCk_Jolt_StaticOccupancyFilter{_Impl->_LayerTable};
        return ck::jolt::Get_IsBoxOccupied(*_Impl->_PhysicsSystem, InCenter, InHalfExtents,
            BroadPhaseFilter, ObjectFilter);
    }

    auto
        FCk_Jolt_CookedWorldQuery::
        Get_BroadphaseBodyCount(
            const FBox& InWorldBounds) const -> int32
    {
        if (NOT Get_IsReady() || InWorldBounds.IsValid == 0)
        { return 0; }

        const auto BroadPhaseFilter = FCk_Jolt_StaticBroadPhaseQueryFilter{};
        const auto ObjectFilter = FCk_Jolt_StaticOccupancyFilter{_Impl->_LayerTable};
        auto BodyIds = TArray<JPH::BodyID>{};
        ck::jolt::Get_BodiesInAABox(*_Impl->_PhysicsSystem, InWorldBounds, BroadPhaseFilter, ObjectFilter, BodyIds);
        return BodyIds.Num();
    }

    auto
        FCk_Jolt_CookedWorldQuery::
        Get_IsSegmentBlocked(
            const FVector& InFrom,
            const FVector& InTo) const -> bool
    {
        using namespace ck_jolt_static_world_cooked_query;

        if (NOT Get_IsReady() || NOT Get_IsFiniteVector(InFrom) || NOT Get_IsFiniteVector(InTo))
        { return false; }

        const auto BroadPhaseFilter = FCk_Jolt_StaticBroadPhaseQueryFilter{};
        const auto ObjectFilter = FCk_Jolt_StaticOccupancyFilter{_Impl->_LayerTable};
        return ck::jolt::Get_IsSegmentBlocked(*_Impl->_PhysicsSystem, InFrom, InTo, BroadPhaseFilter, ObjectFilter);
    }
}

// --------------------------------------------------------------------------------------------------------------------
