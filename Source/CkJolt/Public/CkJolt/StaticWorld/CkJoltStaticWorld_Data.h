#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/CollisionLayers/CkJoltCollisionLayer_Data.h"

#include <Engine/DataAsset.h>
#include <PhysicalMaterials/PhysicalMaterial.h>

#include "CkJoltStaticWorld_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    /// Bump on ANY change to the cooked record/blob format. Mismatching cooked data is ensured
    /// loudly and skipped — never silently reinterpreted.
    constexpr uint32 CookVersion_Current = 1;
}

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

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    TArray<FCk_Jolt_CookedCellRef> _Cells;

    UPROPERTY()
    TMap<FName, FCk_Jolt_CookedActorRef> _ActorLookup;

public:
    CK_PROPERTY(_CookVersion);
    CK_PROPERTY(_JoltVersionId);
    CK_PROPERTY(_SourceMapPackage);
    CK_PROPERTY(_Cells);
    CK_PROPERTY(_ActorLookup);
};

// --------------------------------------------------------------------------------------------------------------------
