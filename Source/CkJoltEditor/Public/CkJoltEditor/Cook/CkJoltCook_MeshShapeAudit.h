#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/StaticWorld/CkJoltBakeExtraction.h"

#include <Containers/Array.h>
#include <Math/Vector.h>

// --------------------------------------------------------------------------------------------------------------------

class UStaticMesh;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::cook
{
    enum class ECk_Jolt_MeshShapeAuditSourceState : uint8
    {
        Ready,
        MissingBodySetup,
        NotWorthPreBaking,
        MissingTriMesh,
        InvalidTriMesh,
        InvalidTriangleIndices,
        UnsupportedCookedPath,
    };

    enum class ECk_Jolt_MeshShapeAuditCookedState : uint8
    {
        Missing,
        Current,
        StaleCookVersion,
        StaleJoltVersion,
        StaleBodySetup,
        StaleTraceFlag,
        Corrupt,
        Orphan,
    };

    /// Why the UI can or cannot render the decoded Jolt mesh itself. This is independent from
    /// CookedState: a stale source header may still have a safely decodable blob for inspection.
    enum class ECk_Jolt_MeshShapeAuditCookedPreviewAvailability : uint8
    {
        MissingCookedAsset,
        IncompatibleStale,
        CorruptBlob,
        NonTriMesh,
        Available,
    };

    /// Value-only decoder gate. Only known same-encoding versions are ever passed to Jolt restore.
    enum class ECk_Jolt_MeshShapeAuditCookedPreviewCompatibility : uint8
    {
        Restorable,
        IncompatibleCookVersion,
        IncompatibleJoltVersion,
    };

    enum class ECk_Jolt_MeshShapeAuditWindingVerdict : uint8
    {
        NotTriMesh,
        NoVerdict,
        Outward,
        InsideOut,
    };

    enum class ECk_Jolt_MeshShapeAuditAction : uint8
    {
        None,
        CookMissing,
        RebuildStale,
        RebuildCorrupt,
        RebuildInsideOut,
        FixSource,
        DeleteOrphan,
    };

    /// Freshness decision shared by the mesh cooker and audit UI. RebuildFromSource is deliberately
    /// not a terminal failure: source serialization still supplies the final fail-closed verdict.
    enum class ECk_Jolt_MeshShapeCurrentBlobFreshness : uint8
    {
        UpToDate,
        RebuildFromSource,
    };

    struct FCk_Jolt_MeshShapeCurrentBlobAuditInput
    {
        bool _bRestored = false;
        bool _bIsTriMesh = false;
        double _TriMeshWindingRatio = 0.0;
    };

    /// One value-only triangle for an editor preview. No source asset or Jolt shape is retained.
    struct FCk_Jolt_MeshShapeAuditTriangle
    {
        FVector _A = FVector::ZeroVector;
        FVector _B = FVector::ZeroVector;
        FVector _C = FVector::ZeroVector;
    };

    /// Capped value-only geometry for an editor preview. Invalid triangle indices are omitted; the
    /// source array and its Jolt-owned memory are never retained.
    struct CKJOLTEDITOR_API FCk_Jolt_MeshShapeAuditPreview
    {
        TArray<FCk_Jolt_MeshShapeAuditTriangle> _Triangles;
        bool _bTruncated = false;
        bool _bUnavailable = false;
    };

    /// Strong winding verdicts use the production baker's +/- 0.05 ratio boundary. Values inside
    /// the closed interval are intentionally ambiguous rather than outward or inside-out.
    CKJOLTEDITOR_API auto Get_MeshShapeAuditWindingVerdict(
        double InWindingRatio) -> ECk_Jolt_MeshShapeAuditWindingVerdict;

    /// v3 is current; v2 predates only the winding correction and retains the same blob encoding.
    /// Every other cook version and every Jolt-version mismatch is fail-closed for preview restore.
    CKJOLTEDITOR_API auto Get_MeshShapeAuditCookedPreviewCompatibility(
        uint32 InCookVersion,
        uint32 InJoltVersionId) -> ECk_Jolt_MeshShapeAuditCookedPreviewCompatibility;

    /// Pure policy for a source-matching current blob. A corrupt blob and a strongly inside-out
    /// tri-mesh both rebuild from the available source; convex/no-verdict blobs remain current.
    CKJOLTEDITOR_API auto Get_MeshShapeCurrentBlobFreshness(
        const FCk_Jolt_MeshShapeCurrentBlobAuditInput& InInput) -> ECk_Jolt_MeshShapeCurrentBlobFreshness;

    /// Pure value copy for a UI preview. InMaxTriangles is clamped to zero or greater.
    CKJOLTEDITOR_API auto Build_MeshShapeAuditPreview(
        const JPH::VertexList& InVertices,
        const JPH::IndexedTriangleList& InTriangles,
        int32 InMaxTriangles) -> FCk_Jolt_MeshShapeAuditPreview;

    /// Copies the restored Jolt tri-mesh through Shape::GetTriangles. Non-mesh shapes do not have a
    /// safe mesh preview contract and return _bUnavailable instead of approximating their geometry.
    CKJOLTEDITOR_API auto Build_MeshShapeAuditPreview(
        const JPH::Shape& InShape,
        int32 InMaxTriangles) -> FCk_Jolt_MeshShapeAuditPreview;

    /// Read-only selected-mesh diagnostic. Analyze_MeshShape never creates/saves packages, changes file
    /// attributes, invalidates runtime caches, or mutates the mesh's source triangle lists.
    struct CKJOLTEDITOR_API FCk_Jolt_MeshShapeAuditResult
    {
        FString _SourceMeshPackagePath;
        FString _CookedShapeObjectPath;
        FString _Failure;

        ECk_Jolt_MeshShapeAuditSourceState _SourceState = ECk_Jolt_MeshShapeAuditSourceState::MissingBodySetup;
        ECk_Jolt_MeshShapeAuditCookedState _CookedState = ECk_Jolt_MeshShapeAuditCookedState::Missing;
        ECk_Jolt_MeshShapeAuditCookedPreviewAvailability _CookedPreviewAvailability =
            ECk_Jolt_MeshShapeAuditCookedPreviewAvailability::MissingCookedAsset;
        ECk_Jolt_MeshShapeAuditWindingVerdict _SourceWinding = ECk_Jolt_MeshShapeAuditWindingVerdict::NotTriMesh;
        ECk_Jolt_MeshShapeAuditWindingVerdict _NormalizedSourceWinding = ECk_Jolt_MeshShapeAuditWindingVerdict::NotTriMesh;
        ECk_Jolt_MeshShapeAuditWindingVerdict _CookedWinding = ECk_Jolt_MeshShapeAuditWindingVerdict::NotTriMesh;
        ECk_Jolt_MeshShapeAuditAction _RecommendedAction = ECk_Jolt_MeshShapeAuditAction::None;

        ck::jolt::bake::FCk_Jolt_WindingNormalizationResult _Normalization;
        double _SourceWindingRatio = 0.0;
        double _NormalizedSourceWindingRatio = 0.0;
        double _CookedWindingRatio = 0.0;
        int32 _SourceTriangleCount = 0;
        int32 _PreviewTriangleLimit = 0;
        int32 _IndividualHeuristicRepairCount = 0;
        int32 _AggregateHeuristicRepairCount = 0;
        bool _bSourcePreviewTruncated = false;
        bool _bNormalizedPreviewTruncated = false;
        bool _bCookedPreviewTruncated = false;
        bool _bCookedPreviewUnavailable = true;
        bool _bWouldUseHeuristic = false;
        bool _bWouldFailBake = false;

        TArray<FCk_Jolt_MeshShapeAuditTriangle> _SourcePreviewTriangles;
        TArray<FCk_Jolt_MeshShapeAuditTriangle> _NormalizedPreviewTriangles;
        TArray<FCk_Jolt_MeshShapeAuditTriangle> _CookedPreviewTriangles;
    };

    /// Reads one selected mesh and its convention-derived cooked asset, then returns diagnostics only.
    /// Owns a ref-counted Jolt-global lease for the full call, including source extraction and preview copies.
    CKJOLTEDITOR_API auto Analyze_MeshShape(
        const UStaticMesh& InMesh,
        int32 InMaxPreviewTriangles = 4096) -> FCk_Jolt_MeshShapeAuditResult;
}

// --------------------------------------------------------------------------------------------------------------------
