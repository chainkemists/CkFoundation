#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <GameplayTagContainer.h>

#include "Ck2dGridCell_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGRID_API FCk_Handle_2dGridCell : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_2dGridCell); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_2dGridCell);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_Fragment_2dGridCell_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_2dGridCell_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "2dGridCell"))
    FGameplayTagContainer _Tags;

public:
    CK_PROPERTY(_Tags);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_2dGridCell_ParamsData, _Tags);
};

// --------------------------------------------------------------------------------------------------------------------