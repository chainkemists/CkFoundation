#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <Math/TransformCalculus2D.h>

#include "Ck2dGridSystem_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGRID_API FCk_Handle_2dGridSystem : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_2dGridSystem); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_2dGridSystem);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_2dGridSystem_CellFilter : uint8
{
    OnlyActiveCells,
    OnlyDisabledCells,
    NoFilter UMETA(DisplayName = "No Filter (All Cells)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_2dGridSystem_CellFilter);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_Fragment_2dGridSystem_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_2dGridSystem_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Dimensions = FIntPoint(1, 1);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector2D _CellSize = FVector2D(100.0f, 100.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSet<FIntPoint> _ActiveCoordinates;

public:
    CK_PROPERTY(_Dimensions);
    CK_PROPERTY(_CellSize);
    CK_PROPERTY(_ActiveCoordinates);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_2dGridSystem_ParamsData, _Dimensions, _CellSize, _ActiveCoordinates);
};

// --------------------------------------------------------------------------------------------------------------------