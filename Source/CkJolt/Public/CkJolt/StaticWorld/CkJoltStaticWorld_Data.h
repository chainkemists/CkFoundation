#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/CollisionLayers/CkJoltCollisionLayer_Data.h"

#include <Engine/DataAsset.h>
#include <PhysicalMaterials/PhysicalMaterial.h>

#include "CkJoltStaticWorld_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    /// Bump on ANY change to the cooked record/blob format OR the bake-admission semantics.
    /// Mismatching cooked data is ensured loudly and skipped — never silently reinterpreted.
    ///
    /// The MAP cook and the MESH-SHAPE cache version SEPARATELY and on purpose. They were one shared
    /// constant until a map-format change forced a needless rewrite of all ~850 shape assets — which
    /// under Git LFS locking means 850 files you may not hold the lock for. Bump only the one whose
    /// format actually moved.

    /// Per-mesh Jolt shape blobs under <CookedRoot>/Meshes. Staleness is ALSO guarded per-asset by the
    /// source BodySetup GUID, so this only needs to move when the blob encoding itself changes.
    /// v2: current shape blob encoding.
    constexpr uint32 MeshShapeCookVersion_Current = 2;

    /// The per-map JoltIndex + JoltCell assets.
    /// v2: settings-driven bake filter (mobility policy + exclusions) changed the baked population.
    /// v3: actor identity is level-qualified. v2 keyed the index by bare actor FName, which is unique
    ///     only WITHIN a level — a streamed map routinely holds one `StaticMeshActor_0` PER SUBLEVEL,
    ///     so every colliding actor but one resolved to a stranger's cooked group, failed the runtime
    ///     hash check, and had its bodies SKIPPED. v2 data cannot be disambiguated after the fact.
    constexpr uint32 WorldCookVersion_Current = 3;
}

// --------------------------------------------------------------------------------------------------------------------

/// Identity of one actor inside a cooked map. The actor NAME alone is NOT an identity — names are
/// unique only within their own level, and a level-streamed map routinely holds a dozen actors all
/// called `StaticMeshActor_0`, one per sublevel. Everything that maps an actor to its cooked record
/// keys on this pair: the cook, the runtime lookup, the incremental plan, and the validators.
struct FCk_Jolt_CookedActorKey
{
    FName _LevelPackage;
    FName _ActorName;

    friend auto operator==(const FCk_Jolt_CookedActorKey& InA, const FCk_Jolt_CookedActorKey& InB) -> bool
    {
        return InA._LevelPackage == InB._LevelPackage && InA._ActorName == InB._ActorName;
    }

    friend auto GetTypeHash(const FCk_Jolt_CookedActorKey& InKey) -> uint32
    {
        return HashCombine(GetTypeHash(InKey._LevelPackage), GetTypeHash(InKey._ActorName));
    }
};

// --------------------------------------------------------------------------------------------------------------------

/// One Jolt body extracted from a source component. Scale is baked into the shape geometry —
/// the transform here is position + rotation only. The signature is resolved to a live
/// JPH::ObjectLayer at load time, NEVER stored as a raw layer index (indices are per-session).
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Jolt_CookedBodyRecord
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Jolt_CookedBodyRecord);

private:
    UPROPERTY()
    int32 _ShapeIndex = INDEX_NONE;

    UPROPERTY()
    FVector _Position = FVector::ZeroVector;

    UPROPERTY()
    FQuat _Rotation = FQuat::Identity;

    UPROPERTY()
    FCk_Jolt_CollisionSignature _Signature;

    UPROPERTY()
    float _Friction = 0.2f;

    UPROPERTY()
    float _Restitution = 0.0f;

    UPROPERTY()
    TEnumAsByte<EPhysicalSurface> _SurfaceType = SurfaceType_Default;

public:
    CK_PROPERTY(_ShapeIndex);
    CK_PROPERTY(_Position);
    CK_PROPERTY(_Rotation);
    CK_PROPERTY(_Signature);
    CK_PROPERTY(_Friction);
    CK_PROPERTY(_Restitution);
    CK_PROPERTY(_SurfaceType);
};

// --------------------------------------------------------------------------------------------------------------------

/// All bodies extracted from one source actor, keyed by the actor's level-relative FName —
/// the runtime streaming-lockstep lookup key. Two hashes serve two audiences:
/// _SourceHash (full, editor-recomputable) drives incremental re-cook decisions;
/// _RuntimeCheckHash (cheap, computable in cooked builds) makes stale data LOUD at load.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Jolt_CookedActorGroup
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Jolt_CookedActorGroup);

private:
    UPROPERTY()
    FName _SourceActorName;

    // The level the actor lives in — the other half of its identity (_SourceActorName alone is unique
    // only within this level). Stored explicitly rather than parsed back out of _SourceActorPath,
    // whose package is the EXTERNAL actor package under One-File-Per-Actor, not the level.
    UPROPERTY()
    FName _SourceLevelPackage;

    // Debug/reporting only — never used for runtime resolution.
    UPROPERTY()
    FSoftObjectPath _SourceActorPath;

    UPROPERTY()
    uint64 _SourceHash = 0;

    UPROPERTY()
    uint64 _RuntimeCheckHash = 0;

    UPROPERTY()
    TArray<FCk_Jolt_CookedBodyRecord> _Bodies;

public:
    CK_PROPERTY(_SourceActorName);
    CK_PROPERTY(_SourceLevelPackage);
    CK_PROPERTY(_SourceActorPath);
    CK_PROPERTY(_SourceHash);
    CK_PROPERTY(_RuntimeCheckHash);
    CK_PROPERTY(_Bodies);
};

// --------------------------------------------------------------------------------------------------------------------

/// One bake-grid cell's worth of cooked Jolt data. The shape blob is a single
/// Shape::SaveWithChildren stream with one shared ShapeToIDMap — shapes shared by N bodies
/// (instanced meshes) are written once and restored as shared JPH::Refs.
UCLASS()
class CKJOLT_API UCk_Jolt_CookedCell_UE : public UDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Jolt_CookedCell_UE);

private:
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    uint32 _CookVersion = 0;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    uint32 _JoltVersionId = 0;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FIntPoint _CellId = FIntPoint::ZeroValue;

    UPROPERTY()
    TArray<uint8> _ShapeBlob;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    int32 _ShapeCount = 0;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    TArray<FCk_Jolt_CookedActorGroup> _ActorGroups;

public:
    CK_PROPERTY(_CookVersion);
    CK_PROPERTY(_JoltVersionId);
    CK_PROPERTY(_CellId);
    CK_PROPERTY(_ShapeBlob);
    CK_PROPERTY(_ShapeCount);
    CK_PROPERTY(_ActorGroups);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Jolt_CookedCellRef
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Jolt_CookedCellRef);

private:
    UPROPERTY()
    FIntPoint _CellId = FIntPoint::ZeroValue;

    UPROPERTY()
    FBox _Bounds = FBox{ForceInit};

    UPROPERTY()
    TSoftObjectPtr<UCk_Jolt_CookedCell_UE> _CellAsset;

public:
    CK_PROPERTY(_CellId);
    CK_PROPERTY(_Bounds);
    CK_PROPERTY(_CellAsset);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Jolt_CookedActorRef
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Jolt_CookedActorRef);

private:
    UPROPERTY()
    int32 _CellIndex = INDEX_NONE;

    UPROPERTY()
    int32 _GroupIndex = INDEX_NONE;

public:
    CK_PROPERTY(_CellIndex);
    CK_PROPERTY(_GroupIndex);
};

// --------------------------------------------------------------------------------------------------------------------

/// The cooked actors of ONE level, by actor name. Nested inside the index's per-level map because
/// UPROPERTY containers cannot nest directly.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Jolt_CookedActorsInLevel
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Jolt_CookedActorsInLevel);

private:
    UPROPERTY()
    TMap<FName, FCk_Jolt_CookedActorRef> _ActorsByName;

public:
    CK_PROPERTY(_ActorsByName);
};

// --------------------------------------------------------------------------------------------------------------------

/// One per pre-baked MESH ASSET: <CookedDataRoot>/Meshes/<MeshPath>_JoltShape. Holds the SCALE-1
/// Jolt shape (SaveWithChildren blob) built from the mesh's BodySetup — runtime wraps it in a
/// JPH::ScaledShape per instance scale, skipping the expensive hull/tri-mesh build. Found by path
/// convention like the map index. Only meshes whose collision includes a convex hull or cooked
/// tri-mesh get one — pure-primitive collision is cheaper to rebuild than to load.
UCLASS()
class CKJOLT_API UCk_Jolt_CookedMeshShape_UE : public UDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Jolt_CookedMeshShape_UE);

private:
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    uint32 _CookVersion = 0;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    uint32 _JoltVersionId = 0;

    // Debug/reporting only — the asset is FOUND by path convention, never by this reference.
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FSoftObjectPath _SourceMesh;

    // Staleness identity: the source BodySetup at cook time. A runtime mismatch means the mesh's
    // collision changed since the cook — the blob is WRONG geometry, ensured loudly and skipped.
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FGuid _BodySetupGuid;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    uint8 _TraceFlag = 0;

    UPROPERTY()
    TArray<uint8> _ShapeBlob;

public:
    CK_PROPERTY(_CookVersion);
    CK_PROPERTY(_JoltVersionId);
    CK_PROPERTY(_SourceMesh);
    CK_PROPERTY(_BodySetupGuid);
    CK_PROPERTY(_TraceFlag);
    CK_PROPERTY(_ShapeBlob);
};

// --------------------------------------------------------------------------------------------------------------------

/// One per baked map: <CookedDataRoot>/<MapPath>/JoltIndex. Found by path convention at runtime
/// (nothing hard-references cooked assets — the cook root must be in DirectoriesToAlwaysCook,
/// which the cook commandlet ensures loudly).
UCLASS()
class CKJOLT_API UCk_Jolt_CookedWorldIndex_UE : public UDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Jolt_CookedWorldIndex_UE);

private:
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    uint32 _CookVersion = 0;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    uint32 _JoltVersionId = 0;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FName _SourceMapPackage;

    // Fingerprint of the bake filter the cook ran under. Compared against the CURRENT settings-built
    // filter at load: cooked data baked under different filter settings is stale for the whole map.
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    uint64 _BakeFilterHash = 0;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    TArray<FCk_Jolt_CookedCellRef> _Cells;

    // Keyed by LEVEL package, then by actor name. Two levels, not one flat map: an actor name is
    // unique only within its level, so a flat name->ref map silently collapses every same-named
    // actor in the map onto ONE cooked group (see CookVersion v3). The outer lookup also happens
    // once per level-add rather than once per actor, so this is cheaper than the flat map it replaced.
    UPROPERTY()
    TMap<FName, FCk_Jolt_CookedActorsInLevel> _ActorLookupByLevel;

public:
    CK_PROPERTY(_CookVersion);
    CK_PROPERTY(_JoltVersionId);
    CK_PROPERTY(_SourceMapPackage);
    CK_PROPERTY(_BakeFilterHash);
    CK_PROPERTY(_Cells);
    CK_PROPERTY(_ActorLookupByLevel);
};

// --------------------------------------------------------------------------------------------------------------------
