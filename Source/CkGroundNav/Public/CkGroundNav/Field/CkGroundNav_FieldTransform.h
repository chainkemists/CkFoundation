#pragma once

#include "CkGroundNav/Field/CkGroundNav_Field.h"

#include <GameplayTagContainer.h>

namespace ck::groundnav
{
enum class ECk_GroundNav_FieldMergeStatus : uint8
{
    Completed,
    InvalidInput,
    IncompatibleLattice,
    OutOfBounds,
    TileOverlap,
    IdentityConflict,
    MissingTagRemap
};

/** Placement and identity translation for one serialized field instance. */
struct CKGROUNDNAV_API FCk_GroundNav_FieldInstanceTransform
{
    FCk_GroundNav_VolumeId _DestinationVolumeId;
    FCk_GroundNav_TileCoord _DestinationTileAnchor;
    int32 _YawDegrees = 0;
    int32 _MarkupIdOffset = 0;
    int32 _LinkIdOffset = 0;
    TMap<FGameplayTag, FGameplayTag> _TagRemap;
};

/** The destination identity and complete value field produced by a successful merge. */
struct CKGROUNDNAV_API FCk_GroundNav_TransformedFieldInstance
{
    FCk_GroundNav_VolumeId _VolumeId;
    FCk_GroundNav_Field _Field;
};

/**
 * Copies a transformed source into the host lattice and re-derives all field-global topology.
 * On every refusal, OutInstance remains byte-identical to its value on entry.
 */
CKGROUNDNAV_API auto TryMerge_TransformedField(const FCk_GroundNav_Field &InHost, const FCk_GroundNav_Field &InSource,
                                               const FCk_GroundNav_FieldInstanceTransform &InDescriptor,
                                               FCk_GroundNav_TransformedFieldInstance &OutInstance)
    -> ECk_GroundNav_FieldMergeStatus;
} // namespace ck::groundnav
