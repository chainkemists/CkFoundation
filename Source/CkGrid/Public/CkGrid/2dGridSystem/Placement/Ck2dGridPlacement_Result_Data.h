#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include "Ck2dGridPlacement_Result_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_2dGridPlacement_Failure : uint8
{
    None,
    OutOfBounds,
    Disabled,
    Occupied,
    TagMismatch,
    Disconnected
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_2dGridPlacement_Failure);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_GridConnectivity : uint8
{
    // Placement validity does NOT consider whether the object connects to existing placements.
    Ignore,
    // Placement is only valid if the object's cells are adjacent to (connected with) existing placements.
    RequireConnected
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GridConnectivity);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_2dGridPlacement_Result
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_2dGridPlacement_Result);

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _CanPlace = false;

    // Every cell that failed validation (NOT just the first). Empty when _CanPlace is true.
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FIntPoint> _FailedCells;

    // The first / most-relevant failure reason encountered while validating.
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_2dGridPlacement_Failure _Reason = ECk_2dGridPlacement_Failure::None;

public:
    CK_PROPERTY_GET(_CanPlace);
    CK_PROPERTY_GET(_FailedCells);
    CK_PROPERTY_GET(_Reason);

public:
    FCk_2dGridPlacement_Result() = default;

    FCk_2dGridPlacement_Result(
        bool InCanPlace,
        ECk_2dGridPlacement_Failure InReason,
        TArray<FIntPoint> InFailedCells)
        : _CanPlace(InCanPlace)
        , _FailedCells(MoveTemp(InFailedCells))
        , _Reason(InReason)
    { }

public:
    static auto
    Success() -> FCk_2dGridPlacement_Result
    {
        return FCk_2dGridPlacement_Result{true, ECk_2dGridPlacement_Failure::None, {}};
    }

    static auto
    Failure(
        ECk_2dGridPlacement_Failure InReason,
        TArray<FIntPoint> InFailedCells) -> FCk_2dGridPlacement_Result
    {
        return FCk_2dGridPlacement_Result{false, InReason, MoveTemp(InFailedCells)};
    }
};

// --------------------------------------------------------------------------------------------------------------------
