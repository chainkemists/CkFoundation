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
struct CKGRID_API FCk_Fragment_2dGridPlacement_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_2dGridPlacement_ParamsData);

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
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_2dGridPlacement_ParamsData, _Occupant, _Grid, _Anchor, _Cells);
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
