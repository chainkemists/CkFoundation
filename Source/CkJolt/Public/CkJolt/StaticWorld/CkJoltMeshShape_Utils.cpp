#include "CkJoltMeshShape_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Data.h"

#include <Engine/StaticMesh.h>
#include <PhysicsEngine/BodySetup.h>

#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>

#include <sstream>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_mesh_shape_utils
{
    struct FCacheEntry
    {
        // Null shape = memoized negative (asset absent or stale) — the disk lookup runs ONCE per
        // mesh per session either way.
        JPH::Ref<JPH::Shape> _ScaleOneShape;
    };

    static TMap<FName, FCacheEntry> Cache;

    static auto Get_ExpectsCookedData() -> bool
    {
#if WITH_EDITOR
        return UCk_Utils_Jolt_ProjectSettings::Get_PIEStaticWorldMode() == ECk_Jolt_PIEStaticWorldMode::Cooked;
#else
        return true;
#endif
    }

    static auto Restore_SingleShapeFromBlob(
        const TArray<uint8>& InBlob,
        const FString& InDebugName) -> JPH::Ref<JPH::Shape>
    {
        auto BlobStream = std::istringstream{
            std::string{reinterpret_cast<const char*>(InBlob.GetData()), static_cast<size_t>(InBlob.Num())}};
        auto StreamWrapper = JPH::StreamInWrapper{BlobStream};

        auto IdToShape = JPH::Shape::IDToShapeMap{};
        auto IdToMaterial = JPH::Shape::IDToMaterialMap{};

        const auto Result = JPH::Shape::sRestoreWithChildren(StreamWrapper, IdToShape, IdToMaterial);

        CK_ENSURE_IF_NOT(Result.IsValid(),
            TEXT("Cooked mesh shape for [{}] failed to restore: [{}] — falling back to a runtime build"),
            InDebugName, FString{Result.GetError().c_str()})
        { return {}; }

        return Result.Get();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::bake::mesh_shape_utils
{
    using namespace ck_jolt_mesh_shape_utils;

    auto
        Get_CookedMeshShapeAssetPath(
            const FString& InCookedDataRootPath,
            const FString& InMeshPackagePath)
        -> FString
    {
        // /Game/Props/SM_Crate -> <Root>/Meshes/Props/SM_Crate_JoltShape.SM_Crate_JoltShape
        auto MeshSubPath = InMeshPackagePath;
        if (NOT MeshSubPath.RemoveFromStart(TEXT("/Game")))
        { return {}; }

        const auto AssetName = FPackageName::GetShortName(InMeshPackagePath);

        return ck::Format_UE(TEXT("{}/Meshes{}_JoltShape.{}_JoltShape"),
            InCookedDataRootPath, MeshSubPath, AssetName);
    }

    auto
        Get_IsWorthPreBaking(
            const UBodySetup& InBodySetup)
        -> bool
    {
        if (InBodySetup.GetCollisionTraceFlag() == CTF_UseComplexAsSimple)
        { return InBodySetup.TriMeshGeometries.Num() > 0; }

        if (InBodySetup.AggGeom.ConvexElems.Num() > 0)
        { return true; }

        // No simple collision at all: the extractor bakes the cooked tri-mesh (Chaos parity).
        const auto HasAnySimple = InBodySetup.AggGeom.GetElementCount() > 0;
        if (NOT HasAnySimple && InBodySetup.TriMeshGeometries.Num() > 0
            && InBodySetup.GetCollisionTraceFlag() != CTF_UseSimpleAsComplex)
        { return true; }

        return false;
    }

    auto
        TryGet_ScaleOneShape(
            const UStaticMesh& InMesh)
        -> JPH::Ref<JPH::Shape>
    {
        const auto MeshPackagePath = InMesh.GetOutermost()->GetName();
        const auto CacheKey = FName{*MeshPackagePath};

        if (const auto* Existing = Cache.Find(CacheKey))
        { return Existing->_ScaleOneShape; }

        // Single map write at the end — LoadObject below can re-enter arbitrary code, and a held
        // element reference would dangle on rehash.
        const auto Memoize = [&](const JPH::Ref<JPH::Shape>& InShape) -> JPH::Ref<JPH::Shape>
        {
            Cache.Add(CacheKey, FCacheEntry{InShape});
            return InShape;
        };

        const auto* BodySetup = InMesh.GetBodySetup();
        if (ck::Is_NOT_Valid(BodySetup))
        { return Memoize({}); }

        const auto AssetPath = Get_CookedMeshShapeAssetPath(
            UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath(), MeshPackagePath);

        // LOAD_NoWarn | LOAD_Quiet: a missing cooked asset is an expected miss (fallback builds
        // from the BodySetup), but the engine's default load path emits LogUObjectGlobals
        // "Failed to find object" + LogStreaming "SkipPackage" warnings that automation test
        // runs capture as failures. Loudness for a miss that SHOULD have been baked stays with
        // the CK_ENSURE below.
        const auto* ShapeAsset = AssetPath.IsEmpty()
            ? nullptr
            : LoadObject<UCk_Jolt_CookedMeshShape_UE>(nullptr, *AssetPath, nullptr, LOAD_NoWarn | LOAD_Quiet);

        if (ck::Is_NOT_Valid(ShapeAsset))
        {
            // Loud ONLY for a mesh the pre-bake should have covered, in a context that expected
            // cooked data — everything else is an expected miss.
            const auto MissShouldEnsure = Get_ExpectsCookedData()
                && Get_IsUnderBakedRoot(MeshPackagePath)
                && Get_IsWorthPreBaking(*BodySetup);

            CK_ENSURE_IF_NOT(NOT MissShouldEnsure,
                TEXT("Mesh [{}] has NO cooked Jolt shape but sits under a BakedMeshShapeRoots root with "
                     "hull/tri-mesh collision — its shape is being built AT RUNTIME. Re-run the Jolt mesh "
                     "cook (or remove the root from the setting if this folder should not be pre-baked)."),
                MeshPackagePath)
            {}

            ck::jolt::Verbose(TEXT("No cooked Jolt mesh shape for [{}] — runtime build"), MeshPackagePath);
            return Memoize({});
        }

        const auto VersionsMatch = ShapeAsset->Get_CookVersion() == ck::jolt::CookVersion_Current
            && ShapeAsset->Get_JoltVersionId() == static_cast<uint32>(JPH_VERSION_ID);
        const auto SourceMatches = ShapeAsset->Get_BodySetupGuid() == BodySetup->BodySetupGuid
            && ShapeAsset->Get_TraceFlag() == static_cast<uint8>(BodySetup->GetCollisionTraceFlag());

        const auto BlobIsUsable = VersionsMatch && SourceMatches;
        const auto MeshIsWorthPreBaking = Get_IsWorthPreBaking(*BodySetup);

        // Drift on a mesh the cook no longer covers is an ORPHAN, not a stale bake: the cook skips a
        // mesh whose collision stopped being worth pre-baking, so it can never refresh this blob and
        // "re-run the cook" would be advice that provably cannot work. Deleting the asset is the only
        // fix, and costs nothing — the shape it holds is one the runtime rebuilds for free.
        CK_ENSURE_IF_NOT(BlobIsUsable || MeshIsWorthPreBaking,
            TEXT("Cooked Jolt shape [{}] is ORPHANED — mesh [{}] no longer has hull or tri-mesh collision, "
                 "so the mesh cook SKIPS it and will never refresh this blob. DELETE the cooked asset; "
                 "re-running the cook does NOT help. The shape is built at runtime meanwhile."),
            AssetPath, MeshPackagePath)
        { return Memoize({}); }

        CK_ENSURE_IF_NOT(BlobIsUsable,
            TEXT("Cooked Jolt shape for mesh [{}] is STALE (cook version [{}] vs [{}], Jolt [{}] vs [{}], "
                 "BodySetup guid/trace-flag drift: [{}]) — the blob is skipped and the shape is built at "
                 "runtime. Re-run the Jolt mesh cook."),
            MeshPackagePath, ShapeAsset->Get_CookVersion(), ck::jolt::CookVersion_Current,
            ShapeAsset->Get_JoltVersionId(), static_cast<uint32>(JPH_VERSION_ID), NOT SourceMatches)
        { return Memoize({}); }

        return Memoize(Restore_SingleShapeFromBlob(ShapeAsset->Get_ShapeBlob(), MeshPackagePath));
    }

    auto
        TryWrap_AtScale(
            const JPH::Ref<JPH::Shape>& InScaleOneShape,
            const FVector& InScale,
            const FString& InDebugName)
        -> JPH::Ref<JPH::Shape>
    {
        if (ck::Is_NOT_Valid(InScaleOneShape))
        { return {}; }

        if (InScale.Equals(FVector::OneVector, KINDA_SMALL_NUMBER))
        { return InScaleOneShape; }

        // Mirrored/degenerate scales take the build path — the baked-in-geometry route has always
        // owned those semantics, and ScaledShape's mirroring rules differ per topology.
        if (InScale.GetMin() <= 0.0)
        { return {}; }

        const auto JoltScale = ck::jolt::Conv(InScale);

        if (NOT InScaleOneShape->IsValidScale(JoltScale))
        {
            ck::jolt::Verbose(TEXT("Scale [{}] is not valid for the cooked shape of [{}] — runtime build"),
                InScale, InDebugName);
            return {};
        }

        const auto Settings = JPH::ScaledShapeSettings{InScaleOneShape.GetPtr(), JoltScale};
        const auto Result = Settings.Create();

        CK_ENSURE_IF_NOT(Result.IsValid(),
            TEXT("ScaledShape creation FAILED for [{}] at scale [{}]: [{}]"),
            InDebugName, InScale, FString{Result.GetError().c_str()})
        { return {}; }

        return Result.Get();
    }

    auto
        Get_IsUnderBakedRoot(
            const FString& InMeshPackagePath)
        -> bool
    {
        const auto Roots = UCk_Utils_Jolt_ProjectSettings::Get_BakedMeshShapeRoots();

        return Roots.ContainsByPredicate([&](const FString& InRoot)
        {
            return InMeshPackagePath.StartsWith(InRoot);
        });
    }

    auto
        Invalidate_CacheForMesh(
            const FString& InMeshPackagePath)
        -> void
    {
        Cache.Remove(FName{*InMeshPackagePath});
    }

    auto
        Reset_CacheForTesting()
        -> void
    {
        Cache.Empty();
    }
}

// --------------------------------------------------------------------------------------------------------------------
