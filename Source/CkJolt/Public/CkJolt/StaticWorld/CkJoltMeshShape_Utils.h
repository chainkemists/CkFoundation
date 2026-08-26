#pragma once

#include "CkCore/Macros/CkMacros.h"

// This API returns JPH::Ref<JPH::Shape> — carry the ck::IsValid support for it with the API so
// every consumer TU sees the same executor specialization (an ODR requirement, not a convenience).
#include "CkJolt/CkJolt_Utils.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

// --------------------------------------------------------------------------------------------------------------------

class UStaticMesh;
class UBodySetup;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::bake
{
    /// Runtime access to the pre-baked per-mesh Jolt shapes (UCk_Jolt_CookedMeshShape_UE, cooked by
    /// CkJoltEditor's mesh-shape sweep). Game thread only, like every Jolt surface in this module.
    ///
    /// The cache holds the restored SCALE-1 root shapes for process lifetime — shapes are immutable
    /// and shared across worlds (PIE multi-world included), and the set is bounded by content.
    namespace mesh_shape_utils
    {
        /// The scale-1 root shape for InMesh, restored from its cooked asset (cached after first
        /// load). Null when: no cooked asset exists at the path convention, the asset is stale
        /// (CookVersion / Jolt version / BodySetupGuid / trace flag mismatch — ensured loudly),
        /// or the blob fails to restore. Callers fall back to building from the BodySetup.
        ///
        /// Miss loudness: an absent asset ensures ONLY when cooked data is expected (packaged, or
        /// PIE in Cooked mode), the mesh sits under a configured _BakedMeshShapeRoots root, AND its
        /// collision would actually have been baked (contains a hull or tri-mesh) — pure-primitive
        /// and out-of-root meshes are expected misses and stay Verbose.
        CKJOLT_API auto TryGet_ScaleOneShape(
            const UStaticMesh& InMesh) -> JPH::Ref<JPH::Shape>;

        /// Wraps InScaleOneShape for InScale: identity scale returns the shape unwrapped; a valid
        /// non-identity scale returns a JPH::ScaledShape. Null when Jolt rejects the scale for this
        /// shape's topology (non-uniform on spheres/capsules, non-uniform across rotated compound
        /// children, non-positive components) — the caller must build from the BodySetup instead,
        /// which bakes the scale into the geometry exactly like the pre-cook path always has.
        /// MakeScaleValid is deliberately NOT used: an approximated scale is silently wrong
        /// collision.
        CKJOLT_API auto TryWrap_AtScale(
            const JPH::Ref<JPH::Shape>& InScaleOneShape,
            const FVector& InScale,
            const FString& InDebugName) -> JPH::Ref<JPH::Shape>;

        /// True when the mesh's collision contains a convex hull or requires the cooked tri-mesh —
        /// the shapes worth pre-baking. Pure-primitive collision (boxes/spheres/sphyls only) is
        /// cheaper to rebuild at runtime than to load. Shared by the cooker's skip rule and the
        /// miss-loudness rule so they can never disagree.
        CKJOLT_API auto Get_IsWorthPreBaking(
            const UBodySetup& InBodySetup) -> bool;

        /// True when the mesh package sits under one of the configured _BakedMeshShapeRoots — the
        /// content the per-mesh pre-bake covers. Shared by the cooker's candidate rule, the on-save
        /// auto-cook filter, and the runtime miss-loudness rule so the three can never disagree.
        CKJOLT_API auto Get_IsUnderBakedRoot(
            const FString& InMeshPackagePath) -> bool;

        /// <Root>/Meshes<MeshSubPath>_JoltShape.<Name>_JoltShape — /Game-rooted meshes only
        /// (engine-content meshes are pure primitives and never pre-baked). Empty string for
        /// unsupported roots.
        CKJOLT_API auto Get_CookedMeshShapeAssetPath(
            const FString& InCookedDataRootPath,
            const FString& InMeshPackagePath) -> FString;

        /// Drops the cached entry for ONE mesh. A cook that rewrites a mesh's shape asset must call
        /// this: the cache memoizes MISSES and STALE results for process lifetime, so without it the
        /// running editor keeps serving the pre-cook answer until it is restarted.
        CKJOLT_API auto Invalidate_CacheForMesh(
            const FString& InMeshPackagePath) -> void;

        /// Restores a cooked shape blob (SaveWithChildren format) or null, ensuring on a corrupt
        /// stream. Exposed for the cooker's up-to-date peek: a pre-winding-fix (v2) blob is stale
        /// only if it actually holds a tri-mesh, and the blob itself is the only place that fact
        /// lives. Requires Jolt globals (Factory) to be registered.
        CKJOLT_API auto TryRestore_ShapeBlob(
            const TArray<uint8>& InBlob,
            const FString& InDebugName) -> JPH::Ref<JPH::Shape>;

        /// The cook version that predates the tri-mesh winding fix. Blobs stamped with it share the
        /// current ENCODING and their convex content is still valid — only tri-mesh blobs from that
        /// cook are inside-out. Shared by the runtime consumer and the cooker's up-to-date rule so
        /// the two can never disagree on what v2 means.
        inline constexpr uint32 PreWindingFixMeshShapeCookVersion = 2;

        /// Test seam: drops every cached root shape and the negative-lookup memo.
        CKJOLT_API auto Reset_CacheForTesting() -> void;
    }
}

// --------------------------------------------------------------------------------------------------------------------
