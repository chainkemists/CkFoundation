#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment_Data.h"

#include "Ck2dGridPlacement_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGRID_API FCk_Handle_2dGridPlacement : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_2dGridPlacement); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_2dGridPlacement);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_2dGridPlacement_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_2dGridPlacement_Spec);

private:
    // The entity that occupies the grid (the thing being placed). Owns the placement's death.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle _Occupant;

    // The grid this placement stamps. Stored so the placement's death-watch can mark it for reconcile.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_2dGridSystem _Grid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FIntPoint _Anchor = FIntPoint::ZeroValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_CardinalRotation _Rotation = ECk_CardinalRotation::None;

    // Resolved grid coordinates this placement occupies.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TArray<FIntPoint> _Cells;

public:
    CK_PROPERTY(_Occupant);
    CK_PROPERTY(_Grid);
    CK_PROPERTY(_Anchor);
    CK_PROPERTY(_Rotation);
    CK_PROPERTY(_Cells);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_2dGridPlacement_Spec, _Occupant, _Grid, _Anchor, _Cells);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_2dGridPlacement_ObjectPlaced,
    FCk_Handle_2dGridSystem, InGrid,
    FCk_Handle, InOccupant,
    const TArray<FIntPoint>&, InCells);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_2dGridPlacement_ObjectRemoved,
    FCk_Handle_2dGridSystem, InGrid,
    FCk_Handle, InOccupant);

// --------------------------------------------------------------------------------------------------------------------

// One replicated placement. Carries everything a client needs to rebuild a placement entity
// ({Occupant, Anchor, Rotation, Cells}). The occupant handle is the identity key for the
// added/removed diff (mirrors FCk_InventoryItem_Spatial_ReplicatedEntry::Get_ItemHandle).
USTRUCT(BlueprintType)
struct CKGRID_API FCk_2dGridPlacement_ReplicatedEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_2dGridPlacement_ReplicatedEntry);

    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle _Occupant;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FIntPoint _Anchor = FIntPoint::ZeroValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_CardinalRotation _Rotation = ECk_CardinalRotation::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TArray<FIntPoint> _Cells;

public:
    CK_PROPERTY_GET(_Occupant);
    CK_PROPERTY(_Anchor);
    CK_PROPERTY(_Rotation);
    CK_PROPERTY(_Cells);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_2dGridPlacement_ReplicatedEntry, _Occupant);
};

// --------------------------------------------------------------------------------------------------------------------

// Replicated container payload pushed by the authority and applied on clients via RegisterLazy.
USTRUCT()
struct CKGRID_API FCk_RepData_2dGridPlacements
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_2dGridPlacements);

    UPROPERTY()
    TArray<FCk_2dGridPlacement_ReplicatedEntry> Placements;
};

// --------------------------------------------------------------------------------------------------------------------
