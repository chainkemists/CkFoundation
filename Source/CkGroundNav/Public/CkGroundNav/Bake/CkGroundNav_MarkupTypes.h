#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkShapes/CkShapes_Common.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "CkGroundNav_MarkupTypes.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * What a markup volume decides about the ground it covers.
 *
 * The two are deliberately not one knob with a sentinel value. A volume that forbids standing and a
 * volume that makes standing expensive are answered by different stages — one demotes a span, the
 * other only prices a leg — and a cost multiplier that meant "blocked" at some magic magnitude would
 * put a walkability decision inside a float comparison nothing downstream can see.
 */
UENUM(BlueprintType)
enum class ECk_GroundNav_MarkupKind : uint8
{
    // Decides whether the covered ground may be stood on at all.
    Walkability,

    // Scales what crossing the covered ground costs, and says nothing about whether it is walkable.
    Cost
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GroundNav_MarkupKind);

// --------------------------------------------------------------------------------------------------------------------

/**
 * One authored markup volume, as a value.
 *
 * Holds no world, no actor and no handle: whatever authored the volume resolves it to a shape and a
 * world transform ONCE, and the bake reduces that to cells. A record that pointed back at its author
 * would make a mid-bake edit representable, and the reduction would then disagree with the field it
 * produced.
 *
 * The id is stable per volume and survives a rebuild, so a field can be diffed against the markup
 * that produced it rather than re-derived to find out what changed.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_GroundNav_MarkupRecord
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_GroundNav_MarkupRecord);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _Id = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_AnyShape _Shape;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FTransform _WorldTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FGameplayTag _AreaTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Enable = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_GroundNav_MarkupKind _Kind = ECk_GroundNav_MarkupKind::Walkability;

    // Ignored entirely by a Walkability record, which carries the identity 1.0 so the two kinds
    // multiply through a shared cost path without a branch.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _CostMultiplier = 1.0f;

    // The build counter this volume was submitted against, as a bare int64 rather than the field's
    // FCk_GroundNav_Epoch: that type is a plain struct declared in Field/CkGroundNav_FieldTypes.h,
    // which is a layer ABOVE this one and which no Bake header may include, and it is not a USTRUCT
    // so it could not be a reflected member here in any case. The value is the epoch's _Value.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int64 _RequestedAtEpoch = 0;

public:
    CK_PROPERTY_GET(_Id);
    CK_PROPERTY_GET(_Shape);
    CK_PROPERTY_GET(_WorldTransform);
    CK_PROPERTY_GET(_Kind);

    CK_PROPERTY(_AreaTag);
    CK_PROPERTY(_Enable);
    CK_PROPERTY(_CostMultiplier);
    CK_PROPERTY(_RequestedAtEpoch);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_GroundNav_MarkupRecord, _Id, _Shape, _WorldTransform, _Kind);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * An INCLUSIVE rectangle of cell indices, matching the bounds a plate carries.
     *
     * Its own type rather than an FIntRect because the engine's is half-open on its max, and the two
     * conventions differing by one cell is exactly the error a closed-square lattice cannot survive.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_CellRect
    {
    public:
        int32 _MinX = 0;
        int32 _MinY = 0;
        int32 _MaxX = 0;
        int32 _MaxY = 0;

    public:
        auto operator==(const FCk_GroundNav_CellRect&) const -> bool = default;

    public:
        auto Get_Width() const -> int32 { return (_MaxX - _MinX) + 1; }
        auto Get_Depth() const -> int32 { return (_MaxY - _MinY) + 1; }
        auto Get_CellCount() const -> int32 { return Get_Width() * Get_Depth(); }

        auto Get_Contains(int32 InX, int32 InY) const -> bool
        {
            return InX >= _MinX && InX <= _MaxX && InY >= _MinY && InY <= _MaxY;
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------
