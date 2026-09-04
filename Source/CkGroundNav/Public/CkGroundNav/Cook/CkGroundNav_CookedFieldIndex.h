#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGroundNav/Cook/CkGroundNav_CookedTile.h"

#include <CoreMinimal.h>
#include <Engine/DataAsset.h>

#include "CkGroundNav_CookedFieldIndex.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Every cooked tile of ONE volume's field, and the identity that says which volume and which bake.
 *
 * Keyed on {the level package the cook ran over, the volume's authored cook key}, for the reason
 * CkJolt's cooked world index keys on its source map plus the actor: a level can hold more than one
 * volume, and nothing else about a volume is stable across a session. A volume whose key is None is
 * runtime-only by definition and no index is ever written for it.
 *
 * READ-ONLY IN THE DETAILS PANEL, not read-only on disk. Every property is VisibleAnywhere rather
 * than EditAnywhere and the cook writes all of them through the setters: what the specifier buys is
 * that a human cannot edit a key, a fingerprint or a tile list into disagreeing with the assets it
 * names, which nothing downstream could detect.
 */
UCLASS()
class CKGROUNDNAV_API UCk_GroundNav_CookedFieldIndex_UE : public UDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GroundNav_CookedFieldIndex_UE);

private:
    /** The level package this field was cooked from, normalised the way Get_PackageLookupKey
     *  normalises one: the cook only ever runs on non-PIE worlds, so a raw PIE name would match
     *  nothing this index was ever filed under. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FName _LevelPackage;

    /** The volume's authored cook key. Never None on an index that exists: a volume carrying None is
     *  runtime-only and no cooked field is written for it. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FName _CookKey;

    /** The INPUT fingerprint of the bake these tiles came out of - the authored half of the identity.
     *  The geometry the bake read is a separate value the cooked form does not carry yet; it belongs
     *  beside this one when a cook can record the world revision it ran against. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    uint64 _Fingerprint = 0;

    /** The blob format every tile below was written under. A reader speaking another one refuses the
     *  whole index rather than loading tiles it would misread. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    int32 _FormatVersion = 0;

    /** The lattice every tile below belongs to, carried here as well as on each tile so a reader can
     *  refuse the collection without loading one of them. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FCk_GroundNav_CookedLatticeKey _LatticeKey;

    /** SOFT references, in the lattice's own tile-index order. Hard ones would load the whole field
     *  with the index, which is the cost the cooked form exists to avoid. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    TArray<TSoftObjectPtr<UCk_GroundNav_CookedTile_UE>> _Tiles;

public:
    CK_PROPERTY_GET(_LevelPackage);
    CK_PROPERTY_SET(_LevelPackage);

    CK_PROPERTY_GET(_CookKey);
    CK_PROPERTY_SET(_CookKey);

    CK_PROPERTY_GET(_Fingerprint);
    CK_PROPERTY_SET(_Fingerprint);

    CK_PROPERTY_GET(_FormatVersion);
    CK_PROPERTY_SET(_FormatVersion);

    CK_PROPERTY_GET(_LatticeKey);
    CK_PROPERTY_SET(_LatticeKey);

    CK_PROPERTY_GET(_Tiles);
    CK_PROPERTY_SET(_Tiles);

public:
    /** Whether a reader speaking InFormatVersion may read the tiles this index names. EXACT equality,
     *  for the reason a tile's own answer is exact. */
    auto Get_IsCompatibleWith(int32 InFormatVersion) const -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * Where a cooked field lives, BY CONVENTION - nothing hard-references a cooked asset, and a level
     * that was never cooked simply has nothing at the path.
     *
     * The shape mirrors CkJolt's cooked world index: the level package's path under the content root,
     * then the asset. The volume's cook key is in the asset NAME rather than in the directory, so one
     * level's volumes sit beside each other and a listing of the directory reads as the set of fields
     * that level cooked.
     */
    CKGROUNDNAV_API auto
    Get_CookedIndexAssetPath(
        const FString& InCookedDataRootPath,
        const FString& InLevelPackageName,
        FName          InCookKey) -> FString;

    /** One tile of that field, named by its coord in the lattice. */
    CKGROUNDNAV_API auto
    Get_CookedTileAssetPath(
        const FString& InCookedDataRootPath,
        const FString& InLevelPackageName,
        FName          InCookKey,
        FIntPoint      InTileCoord) -> FString;

    /**
     * Normalises a package name into the form the COOK recorded.
     *
     * PIE renames every level package to /Game/Path/UEDPIE_<N>_Name and the cook only ever runs on
     * non-PIE worlds, so a raw PIE name matches nothing and the lookup would silently degrade to a
     * runtime bake. Everything that looks cooked data up by package name goes through here.
     */
    CKGROUNDNAV_API auto
    Get_PackageLookupKey(
        const FString& InPackageName) -> FName;

    /**
     * The content root cooked ground-nav data is written under.
     *
     * A CONSTANT rather than a project setting: CkGroundNav has no project settings object to hang one
     * on, and inventing one to hold a single path is a decision this does not get to make. It belongs
     * on a settings object beside CkJolt's own _CookedDataRootPath the moment the module has one.
     */
    inline constexpr auto kCookedDataRootPath = TEXT("/Game/CkGroundNavData");
}

// --------------------------------------------------------------------------------------------------------------------
