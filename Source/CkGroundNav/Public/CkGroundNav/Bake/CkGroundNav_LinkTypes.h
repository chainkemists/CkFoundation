#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "CkGroundNav_LinkTypes.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Which way a link may be traversed.
 *
 * A one-way link is a distinct value rather than a pair of cost multipliers with one set impassably
 * high: a magnitude that meant "not traversable" would put a connectivity decision inside a float
 * comparison nothing downstream can see.
 */
UENUM(BlueprintType)
enum class ECk_GroundNav_LinkDirection : uint8
{
    // Traversable start to end and end to start.
    Bidirectional,

    // Traversable start to end only.
    Forward,

    // Traversable end to start only.
    Backward
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GroundNav_LinkDirection);

// --------------------------------------------------------------------------------------------------------------------

/**
 * One authored navigation link, as a value.
 *
 * A link is authored as two WORLD POINTS, and each is projected onto the field at every publish. It
 * stores no plate and no cell: a plate index is valid only against the field it was derived on, so a
 * record carrying one would go quietly wrong on the first rebuild that renumbered it, and the
 * resolution is kept as the bake's separate answer.
 *
 * The id is stable per volume and never reused, so a field can be diffed against the links that
 * produced it rather than re-derived to find out what changed, and a retired id never comes back
 * meaning a different link.
 *
 * The two multipliers price the link's own straight-line span and are never below one, which is what
 * keeps every edge costing at least its Euclidean length — the property the search's Euclidean
 * heuristic is admissible under.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_GroundNav_LinkRecord
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_GroundNav_LinkRecord);

public:
    // Clearance no agent can exceed, so a link that names no width admits every body the plates at
    // its ends already admit.
    static constexpr float kAdmitsAnyAgentClearanceUu = 1.0e6f;

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _Id = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _Start = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _End = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_GroundNav_LinkDirection _Direction = ECk_GroundNav_LinkDirection::Bidirectional;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _CostMultiplierForward = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _CostMultiplierBackward = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _ClearanceUu = kAdmitsAnyAgentClearanceUu;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FGameplayTag _AreaTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FGameplayTag _UserTypeTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Enable = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_NavSurface_ProjectionMode _ProjectionMode = ECk_NavSurface_ProjectionMode::Closest;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _ProjectionHorizontalExtentUu = 50.0f;

    // One number for both directions: an endpoint is authored at the height a body stands at, and a
    // reach that differed up from down would place the same point on two different storeys depending
    // on which end of the link it was.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _ProjectionVerticalExtentUu = 100.0f;

    // The build counter this link was submitted against, as a bare int64 rather than the field's
    // FCk_GroundNav_Epoch: that type is a plain struct declared in Field/CkGroundNav_FieldTypes.h,
    // which is a layer ABOVE this one and which no Bake header may include, and it is not a USTRUCT
    // so it could not be a reflected member here in any case. The value is the epoch's _Value.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int64 _RequestedAtEpoch = 0;

public:
    CK_PROPERTY_GET(_Id);
    CK_PROPERTY_GET(_Start);
    CK_PROPERTY_GET(_End);

    CK_PROPERTY(_Direction);
    CK_PROPERTY(_CostMultiplierForward);
    CK_PROPERTY(_CostMultiplierBackward);
    CK_PROPERTY(_ClearanceUu);
    CK_PROPERTY(_AreaTag);
    CK_PROPERTY(_UserTypeTag);
    CK_PROPERTY(_Enable);
    CK_PROPERTY(_ProjectionMode);
    CK_PROPERTY(_ProjectionHorizontalExtentUu);
    CK_PROPERTY(_ProjectionVerticalExtentUu);
    CK_PROPERTY(_RequestedAtEpoch);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_GroundNav_LinkRecord, _Id, _Start, _End);
};

// --------------------------------------------------------------------------------------------------------------------
