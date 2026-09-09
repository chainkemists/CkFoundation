#pragma once

#include "CkGroundNav/Field/CkGroundNav_FieldSerialize.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    enum class ECk_GroundNav_StreamTileTransitionKind : uint8
    {
        Replace,
        Remove
    };

    struct CKGROUNDNAV_API FCk_GroundNav_StreamTileTransition
    {
    public:
        FCk_GroundNav_StreamTileId _TileId;

        ECk_GroundNav_StreamTileTransitionKind _Kind = ECk_GroundNav_StreamTileTransitionKind::Replace;

        // Replace carries one default SingleTile blob plus exactly one blob for every variant key.
        // Remove carries neither kind of blob.
        TArray<uint8> _DefaultBlob;
        TMap<FGameplayTag, TArray<uint8>> _VariantBlobs;
    };

    /** All fields that must publish together for one logical streamed volume. */
    struct CKGROUNDNAV_API FCk_GroundNav_StreamFieldBundle
    {
    public:
        FCk_GroundNav_Field _DefaultField;
        TMap<FGameplayTag, FCk_GroundNav_Field> _VariantFields;
    };

    enum class ECk_GroundNav_StreamCompositionStatus : uint8
    {
        Composed,
        InvalidVolumeId,
        InvalidTileId,
        InvalidTransition,
        DuplicateTileCoord,
        TileCoordOutsideLattice,
        TileCoordMismatch,
        BlobRefused,
        NoChange
    };

    struct CKGROUNDNAV_API FCk_GroundNav_StreamCompositionResult
    {
    public:
        ECk_GroundNav_StreamCompositionStatus _Status = ECk_GroundNav_StreamCompositionStatus::Composed;

        ECk_GroundNav_LoadStatus _LoadStatus = ECk_GroundNav_LoadStatus::Loaded;

        FGameplayTag _ProfileTag;

    public:
        auto Get_Succeeded() const -> bool
        {
            return _Status == ECk_GroundNav_StreamCompositionStatus::Composed;
        }

        auto Get_HasChanges() const -> bool { return Get_Succeeded(); }
    };

    /** Apply one same-volume, fixed-lattice all-profile transaction. Refusal and NoChange leave
     *  OutBundle untouched; success assigns only a fully composed default-plus-exact-variants bundle. */
    CKGROUNDNAV_API auto
    Compose_StreamTileTransitions(
        const FCk_GroundNav_StreamFieldBundle&        InBaseBundle,
        FCk_GroundNav_VolumeId                        InVolumeId,
        TConstArrayView<FCk_GroundNav_StreamTileTransition> InTransitions,
        FCk_GroundNav_StreamFieldBundle&              OutBundle) -> FCk_GroundNav_StreamCompositionResult;
}

// --------------------------------------------------------------------------------------------------------------------
