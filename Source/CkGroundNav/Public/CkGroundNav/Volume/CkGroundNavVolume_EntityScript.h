#pragma once

#include "CkEcs/EntityScript/CkGenericEntityScript.h"

#include "CkGroundNav/Volume/CkGroundNavVolume_Fragment_Data.h"

#include "CkGroundNavVolume_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * One volume's authored params PLACED at a world transform.
     *
     * The bounds are authored LOCAL to the placement's TRANSLATION and are moved by it; ROTATION AND
     * SCALE ARE IGNORED, because _VolumeBounds is an axis-aligned box and a rotated one is not
     * representable in it. Rotating a placed volume would either have to bake ground the author cannot
     * see or silently grow the box to the rotated one's AABB, and both are worse than saying so.
     *
     * The IDENTITY transform answers the authored params UNCHANGED, which is exactly what every
     * runtime caller that composes a volume without a placement has always passed - so this function
     * cannot move ground that was authored in world space to begin with.
     *
     * ONE placement for BOTH drivers: the EntityScript's Construct calls it with the transform the
     * spawner injected, and the cook commandlet calls it with the same spawner's actor transform. Two
     * placements would be two answers to where a cooked volume's ground is, and the cook's answer is
     * the one nothing at runtime can check.
     */
    CKGROUNDNAV_API auto
    Get_PlacedVolumeParams(
        const FCk_Fragment_GroundNavVolume_ParamsData& InParams,
        const FTransform&                              InPlacement)
        -> FCk_Fragment_GroundNavVolume_ParamsData;
}

// --------------------------------------------------------------------------------------------------------------------

/** What a RUNTIME caller hands the volume EntityScript. A level-placed spawner passes nothing and is
 *  answered by the script's own _Params and the transform the spawner injects into it. */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_GroundNavVolume_SpawnParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GroundNavVolume_SpawnParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Fragment_GroundNavVolume_ParamsData _Params;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FTransform _SpawnTransform = FTransform::Identity;

public:
    CK_PROPERTY_GET(_Params);
    CK_PROPERTY_GET(_SpawnTransform);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_GroundNavVolume_SpawnParams, _Params, _SpawnTransform);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * A ground-nav volume AUTHORED IN A LEVEL: drop an ACk_EntitySpawner_UE, point its EntityScript at
 * this class, and the actor's location places the volume.
 *
 * The bounds authored below are LOCAL TO THE SPAWNER'S TRANSLATION and the actor's rotation and scale
 * are ignored - see ck::groundnav::Get_PlacedVolumeParams, which is the one placement both this script
 * and the cook commandlet apply.
 *
 * A volume that authors a _CookKey is the one the cook commandlet writes a cooked field for; None
 * means runtime-only, which is what every gym and test volume stays.
 */
UCLASS(Blueprintable, BlueprintType)
class CKGROUNDNAV_API UCk_GroundNavVolume_EntityScript : public UCk_GenericEntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GroundNavVolume_EntityScript);

protected:
    UCk_GroundNavVolume_EntityScript(const FObjectInitializer& InObjectInitializer);

private:
    /** The volume's bake settings. _VolumeBounds is authored LOCAL to the spawner's TRANSLATION: the
     *  actor's location moves the box, and its rotation and scale are IGNORED because the box is
     *  axis-aligned. An identity transform leaves the bounds exactly as authored. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Nav Volume",
        meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    FCk_Fragment_GroundNavVolume_ParamsData _Params;

    // EntitySpawner injects its actor transform here for level placement (the default-name fallback in
    // ck::entityspawner::TryResolveDefaultTransformProperty resolves this property). Runtime callers
    // pass FCk_GroundNavVolume_SpawnParams instead.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Nav Volume",
        meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    FTransform _SpawnTransform = FTransform::Identity;

public:
    // Read by the COOK, which reaches the script object on a loaded map where BeginPlay never ran -
    // so it reads these two and the spawner's own actor transform rather than the injected copy.
    CK_PROPERTY_GET(_Params);
    CK_PROPERTY_GET(_SpawnTransform);

private:
    // EntitySpawner injects the source level before duplicating this script for construction.
    UPROPERTY()
    FName _SpawnLevelPackage;

public:
    CK_PROPERTY_GET(_SpawnLevelPackage);

protected:
    auto Construct(FCk_Handle& InHandle, const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;
};

// --------------------------------------------------------------------------------------------------------------------
