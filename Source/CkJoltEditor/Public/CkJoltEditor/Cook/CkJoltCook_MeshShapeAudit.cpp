#include "CkJoltCook_MeshShapeAudit.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJolt/StaticWorld/CkJoltMeshShape_Utils.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Data.h"

#include <Engine/StaticMesh.h>
#include <PhysicsEngine/BodySetup.h>

#include <Jolt/Jolt.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_cook_mesh_shape_audit
{
    using namespace ck::jolt;
    using namespace ck::jolt::bake;

    constexpr auto WindingVerdictThreshold = 0.05;

    static auto Get_SourceState(
        ECk_Jolt_TriMeshExtractionStatus InStatus) -> ck::jolt::cook::ECk_Jolt_MeshShapeAuditSourceState
    {
        switch (InStatus)
        {
            case ECk_Jolt_TriMeshExtractionStatus::Success:
                return ck::jolt::cook::ECk_Jolt_MeshShapeAuditSourceState::Ready;
            case ECk_Jolt_TriMeshExtractionStatus::MissingTriMesh:
                return ck::jolt::cook::ECk_Jolt_MeshShapeAuditSourceState::MissingTriMesh;
            case ECk_Jolt_TriMeshExtractionStatus::InvalidTriMesh:
                return ck::jolt::cook::ECk_Jolt_MeshShapeAuditSourceState::InvalidTriMesh;
            case ECk_Jolt_TriMeshExtractionStatus::InvalidTriangleIndices:
                return ck::jolt::cook::ECk_Jolt_MeshShapeAuditSourceState::InvalidTriangleIndices;
        }

        return ck::jolt::cook::ECk_Jolt_MeshShapeAuditSourceState::InvalidTriMesh;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::cook
{
    auto
        Get_MeshShapeAuditWindingVerdict(
            double InWindingRatio)
        -> ECk_Jolt_MeshShapeAuditWindingVerdict
    {
        using namespace ck_jolt_cook_mesh_shape_audit;

        if (InWindingRatio < -WindingVerdictThreshold)
        { return ECk_Jolt_MeshShapeAuditWindingVerdict::InsideOut; }
        if (InWindingRatio > WindingVerdictThreshold)
        { return ECk_Jolt_MeshShapeAuditWindingVerdict::Outward; }
        return ECk_Jolt_MeshShapeAuditWindingVerdict::NoVerdict;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_MeshShapeAuditCookedPreviewCompatibility(
            uint32 InCookVersion,
            uint32 InJoltVersionId)
        -> ECk_Jolt_MeshShapeAuditCookedPreviewCompatibility
    {
        if (InJoltVersionId != static_cast<uint32>(JPH_VERSION_ID))
        { return ECk_Jolt_MeshShapeAuditCookedPreviewCompatibility::IncompatibleJoltVersion; }

        const auto UsesKnownBlobEncoding = InCookVersion == MeshShapeCookVersion_Current
            || InCookVersion == ck::jolt::bake::mesh_shape_utils::PreWindingFixMeshShapeCookVersion;
        return UsesKnownBlobEncoding
            ? ECk_Jolt_MeshShapeAuditCookedPreviewCompatibility::Restorable
            : ECk_Jolt_MeshShapeAuditCookedPreviewCompatibility::IncompatibleCookVersion;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Build_MeshShapeAuditPreview(
            const JPH::VertexList& InVertices,
            const JPH::IndexedTriangleList& InTriangles,
            int32 InMaxTriangles)
        -> FCk_Jolt_MeshShapeAuditPreview
    {
        auto Result = FCk_Jolt_MeshShapeAuditPreview{};
        const auto MaxTriangles = FMath::Max(0, InMaxTriangles);
        Result._bTruncated = static_cast<int32>(InTriangles.size()) > MaxTriangles;
        Result._Triangles.Reserve(FMath::Min(static_cast<int32>(InTriangles.size()), MaxTriangles));

        const auto NumVertices = static_cast<uint32>(InVertices.size());
        for (auto TriangleIndex = 0; TriangleIndex < static_cast<int32>(InTriangles.size())
            && TriangleIndex < MaxTriangles; ++TriangleIndex)
        {
            const auto& Indices = InTriangles[TriangleIndex].mIdx;
            if (Indices[0] >= NumVertices || Indices[1] >= NumVertices || Indices[2] >= NumVertices)
            { continue; }

            const auto ToVector = [&](uint32 InVertexIndex) -> FVector
            {
                const auto& Vertex = InVertices[InVertexIndex];
                return FVector{Vertex.x, Vertex.y, Vertex.z};
            };

            Result._Triangles.Emplace(FCk_Jolt_MeshShapeAuditTriangle{
                ToVector(Indices[0]), ToVector(Indices[1]), ToVector(Indices[2])});
        }

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_MeshShapeCurrentBlobFreshness(
            const FCk_Jolt_MeshShapeCurrentBlobAuditInput& InInput)
        -> ECk_Jolt_MeshShapeCurrentBlobFreshness
    {
        if (NOT InInput._bRestored)
        { return ECk_Jolt_MeshShapeCurrentBlobFreshness::RebuildFromSource; }

        const auto IsInsideOut = InInput._bIsTriMesh
            && Get_MeshShapeAuditWindingVerdict(InInput._TriMeshWindingRatio)
                == ECk_Jolt_MeshShapeAuditWindingVerdict::InsideOut;
        return IsInsideOut
            ? ECk_Jolt_MeshShapeCurrentBlobFreshness::RebuildFromSource
            : ECk_Jolt_MeshShapeCurrentBlobFreshness::UpToDate;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Build_MeshShapeAuditPreview(
            const JPH::Shape& InShape,
            int32 InMaxTriangles)
        -> FCk_Jolt_MeshShapeAuditPreview
    {
        auto Result = FCk_Jolt_MeshShapeAuditPreview{};
        if (InShape.GetSubType() != JPH::EShapeSubType::Mesh)
        {
            Result._bUnavailable = true;
            return Result;
        }

        const auto MaxTriangles = FMath::Max(0, InMaxTriangles);
        auto Context = JPH::Shape::GetTrianglesContext{};
        InShape.GetTrianglesStart(Context, JPH::AABox::sBiggest(),
            JPH::Vec3::sZero(), JPH::Quat::sIdentity(), JPH::Vec3::sReplicate(1.0f));

        constexpr auto BatchSize = 256;
        static_assert(BatchSize >= JPH::Shape::cGetTrianglesMinTrianglesRequested);
        auto TriangleVertices = TArray<JPH::Float3>{};
        TriangleVertices.SetNumUninitialized(BatchSize * 3);

        for (;;)
        {
            const auto NumTriangles = InShape.GetTrianglesNext(Context, BatchSize, TriangleVertices.GetData());
            if (NumTriangles <= 0)
            { return Result; }

            for (auto TriangleIndex = 0; TriangleIndex < NumTriangles; ++TriangleIndex)
            {
                if (Result._Triangles.Num() >= MaxTriangles)
                {
                    Result._bTruncated = true;
                    return Result;
                }

                const auto& A = TriangleVertices[TriangleIndex * 3];
                const auto& B = TriangleVertices[TriangleIndex * 3 + 1];
                const auto& C = TriangleVertices[TriangleIndex * 3 + 2];
                Result._Triangles.Emplace(FCk_Jolt_MeshShapeAuditTriangle{
                    FVector{A.x, A.y, A.z}, FVector{B.x, B.y, B.z}, FVector{C.x, C.y, C.z}});
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Analyze_MeshShape(
            const UStaticMesh& InMesh,
            int32 InMaxPreviewTriangles)
        -> FCk_Jolt_MeshShapeAuditResult
    {
        using namespace ck_jolt_cook_mesh_shape_audit;
        using namespace ck::jolt;
        using namespace ck::jolt::bake;

        // Extraction and preview allocation use Jolt-owned STL containers even when no cooked blob
        // exists. The public audit owns one ref-counted lease for its full call so a single-row
        // inspector request cannot rely on a batch caller having initialized Jolt already.
        const FCk_Jolt_ScopedGlobalInit ScopedJolt{};
        auto Result = FCk_Jolt_MeshShapeAuditResult{};
        Result._PreviewTriangleLimit = FMath::Max(0, InMaxPreviewTriangles);
        Result._SourceMeshPackagePath = InMesh.GetOutermost()->GetName();
        Result._CookedShapeObjectPath = mesh_shape_utils::Get_CookedMeshShapeAssetPath(
            UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath(), Result._SourceMeshPackagePath);

        const auto* BodySetup = InMesh.GetBodySetup();
        if (ck::Is_NOT_Valid(BodySetup))
        {
            Result._Failure = TEXT("The selected mesh has no BodySetup");
            Result._RecommendedAction = ECk_Jolt_MeshShapeAuditAction::FixSource;
            Result._bWouldFailBake = true;
            return Result;
        }

        const auto IsWorthPreBaking = mesh_shape_utils::Get_IsWorthPreBaking(*BodySetup);
        if (NOT IsWorthPreBaking)
        { Result._SourceState = ECk_Jolt_MeshShapeAuditSourceState::NotWorthPreBaking; }

        if (IsWorthPreBaking && Result._CookedShapeObjectPath.IsEmpty())
        {
            Result._SourceState = ECk_Jolt_MeshShapeAuditSourceState::UnsupportedCookedPath;
            Result._Failure = TEXT("The selected mesh has no supported cooked-shape package path");
            Result._RecommendedAction = ECk_Jolt_MeshShapeAuditAction::FixSource;
            Result._bWouldFailBake = true;
            return Result;
        }

        const auto* CookedAsset = Result._CookedShapeObjectPath.IsEmpty()
            ? nullptr
            : LoadObject<UCk_Jolt_CookedMeshShape_UE>(nullptr, *Result._CookedShapeObjectPath, nullptr,
                LOAD_NoWarn | LOAD_Quiet);
        auto CurrentBlobFreshness = ECk_Jolt_MeshShapeCurrentBlobFreshness::UpToDate;

        if (ck::Is_NOT_Valid(CookedAsset))
        {
            Result._CookedState = ECk_Jolt_MeshShapeAuditCookedState::Missing;
            Result._RecommendedAction = IsWorthPreBaking
                ? ECk_Jolt_MeshShapeAuditAction::CookMissing
                : ECk_Jolt_MeshShapeAuditAction::None;
        }
        else
        {
            const auto CookVersionMatches = CookedAsset->Get_CookVersion() == MeshShapeCookVersion_Current;
            const auto JoltVersionMatches = CookedAsset->Get_JoltVersionId() == static_cast<uint32>(JPH_VERSION_ID);
            const auto BodySetupMatches = CookedAsset->Get_BodySetupGuid() == BodySetup->BodySetupGuid;
            const auto TraceFlagMatches = CookedAsset->Get_TraceFlag()
                == static_cast<uint8>(BodySetup->GetCollisionTraceFlag());

            if (NOT IsWorthPreBaking)
            {
                Result._CookedState = ECk_Jolt_MeshShapeAuditCookedState::Orphan;
                Result._RecommendedAction = ECk_Jolt_MeshShapeAuditAction::DeleteOrphan;
            }
            else if (NOT CookVersionMatches)
            { Result._CookedState = ECk_Jolt_MeshShapeAuditCookedState::StaleCookVersion; }
            else if (NOT JoltVersionMatches)
            { Result._CookedState = ECk_Jolt_MeshShapeAuditCookedState::StaleJoltVersion; }
            else if (NOT BodySetupMatches)
            { Result._CookedState = ECk_Jolt_MeshShapeAuditCookedState::StaleBodySetup; }
            else if (NOT TraceFlagMatches)
            { Result._CookedState = ECk_Jolt_MeshShapeAuditCookedState::StaleTraceFlag; }
            else
            { Result._CookedState = ECk_Jolt_MeshShapeAuditCookedState::Current; }

            const auto PreviewCompatibility = Get_MeshShapeAuditCookedPreviewCompatibility(
                CookedAsset->Get_CookVersion(), CookedAsset->Get_JoltVersionId());
            if (PreviewCompatibility != ECk_Jolt_MeshShapeAuditCookedPreviewCompatibility::Restorable)
            {
                Result._CookedPreviewAvailability =
                    ECk_Jolt_MeshShapeAuditCookedPreviewAvailability::IncompatibleStale;
            }
            else
            {
                const auto Restore = mesh_shape_utils::Restore_ShapeBlobForAnalysis(CookedAsset->Get_ShapeBlob());
                if (NOT Restore._Success)
                {
                    if (Result._CookedState == ECk_Jolt_MeshShapeAuditCookedState::Current)
                    { Result._CookedState = ECk_Jolt_MeshShapeAuditCookedState::Corrupt; }
                    Result._CookedPreviewAvailability = ECk_Jolt_MeshShapeAuditCookedPreviewAvailability::CorruptBlob;
                    Result._Failure = Restore._Failure;
                }
                else
                {
                    const auto IsTriMesh = Restore._Shape->GetSubType() == JPH::EShapeSubType::Mesh;
                    Result._CookedWindingRatio = IsTriMesh ? ComputeShapeWindingRatio(*Restore._Shape) : 0.0;
                    Result._CookedWinding = IsTriMesh
                        ? Get_MeshShapeAuditWindingVerdict(Result._CookedWindingRatio)
                        : ECk_Jolt_MeshShapeAuditWindingVerdict::NotTriMesh;
                    CurrentBlobFreshness = Get_MeshShapeCurrentBlobFreshness(
                        {true, IsTriMesh, Result._CookedWindingRatio});
                    auto CookedPreview = Build_MeshShapeAuditPreview(*Restore._Shape, Result._PreviewTriangleLimit);
                    Result._CookedPreviewTriangles = MoveTemp(CookedPreview._Triangles);
                    Result._bCookedPreviewTruncated = CookedPreview._bTruncated;
                    Result._bCookedPreviewUnavailable = CookedPreview._bUnavailable;
                    Result._CookedPreviewAvailability = CookedPreview._bUnavailable
                        ? ECk_Jolt_MeshShapeAuditCookedPreviewAvailability::NonTriMesh
                        : ECk_Jolt_MeshShapeAuditCookedPreviewAvailability::Available;
                }
            }

            if (Result._CookedPreviewAvailability == ECk_Jolt_MeshShapeAuditCookedPreviewAvailability::CorruptBlob
                && Result._CookedState != ECk_Jolt_MeshShapeAuditCookedState::Orphan)
            { Result._RecommendedAction = ECk_Jolt_MeshShapeAuditAction::RebuildCorrupt; }
            else if (Result._CookedState != ECk_Jolt_MeshShapeAuditCookedState::Current
                && Result._CookedState != ECk_Jolt_MeshShapeAuditCookedState::Orphan)
            { Result._RecommendedAction = ECk_Jolt_MeshShapeAuditAction::RebuildStale; }
            else if (Result._CookedState == ECk_Jolt_MeshShapeAuditCookedState::Current
                && CurrentBlobFreshness == ECk_Jolt_MeshShapeCurrentBlobFreshness::RebuildFromSource)
            { Result._RecommendedAction = ECk_Jolt_MeshShapeAuditAction::RebuildInsideOut; }
        }

        if (NOT IsWorthPreBaking)
        { return Result; }

        if (BodySetup->TriMeshGeometries.IsEmpty())
        {
            if (Result._SourceState != ECk_Jolt_MeshShapeAuditSourceState::NotWorthPreBaking)
            { Result._SourceState = ECk_Jolt_MeshShapeAuditSourceState::Ready; }
            return Result;
        }

        auto Geometry = Extract_TriMeshGeometry(*BodySetup, FVector::OneVector);
        Result._SourceState = Get_SourceState(Geometry._Status);
        if (NOT Geometry.Get_IsValid())
        {
            Result._Failure = TEXT("The source cooked tri-mesh could not be extracted");
            Result._RecommendedAction = ECk_Jolt_MeshShapeAuditAction::FixSource;
            Result._bWouldFailBake = true;
            return Result;
        }

        Result._SourceTriangleCount = static_cast<int32>(Geometry._Triangles.size());
        Result._SourceWindingRatio = ComputeMeshWindingRatio(Geometry._Vertices, Geometry._Triangles);
        Result._SourceWinding = Get_MeshShapeAuditWindingVerdict(Result._SourceWindingRatio);
        auto SourcePreview = Build_MeshShapeAuditPreview(
            Geometry._Vertices, Geometry._Triangles, Result._PreviewTriangleLimit);
        Result._SourcePreviewTriangles = MoveTemp(SourcePreview._Triangles);
        Result._bSourcePreviewTruncated = SourcePreview._bTruncated;

        auto NormalizedTriangles = Geometry._Triangles;
        Result._Normalization = NormalizeInsideOutMeshComponents(Geometry._Vertices, NormalizedTriangles);
        Result._IndividualHeuristicRepairCount = Result._Normalization._NumRepairedComponents;
        Result._AggregateHeuristicRepairCount = Result._Normalization._NumAggregateNoVerdictComponentsRepaired;
        Result._NormalizedSourceWindingRatio = ComputeMeshWindingRatio(Geometry._Vertices, NormalizedTriangles);
        Result._NormalizedSourceWinding = Get_MeshShapeAuditWindingVerdict(Result._NormalizedSourceWindingRatio);
        auto NormalizedPreview = Build_MeshShapeAuditPreview(
            Geometry._Vertices, NormalizedTriangles, Result._PreviewTriangleLimit);
        Result._NormalizedPreviewTriangles = MoveTemp(NormalizedPreview._Triangles);
        Result._bNormalizedPreviewTruncated = NormalizedPreview._bTruncated;

        Result._bWouldUseHeuristic = Result._IndividualHeuristicRepairCount > 0
            || Result._AggregateHeuristicRepairCount > 0;
        Result._bWouldFailBake = Result._Normalization.Get_HasMalformedGeometry()
            || Result._NormalizedSourceWinding == ECk_Jolt_MeshShapeAuditWindingVerdict::InsideOut;

        if (Result._bWouldFailBake)
        { Result._RecommendedAction = ECk_Jolt_MeshShapeAuditAction::FixSource; }
        else if (Result._CookedState == ECk_Jolt_MeshShapeAuditCookedState::Current
            && Result._CookedWinding == ECk_Jolt_MeshShapeAuditWindingVerdict::InsideOut)
        { Result._RecommendedAction = ECk_Jolt_MeshShapeAuditAction::RebuildInsideOut; }

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
