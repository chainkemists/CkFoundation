#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include "CkInventoryItem_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKINVENTORY_API FCk_Handle_Item : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Item); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Item);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Fragment_InventoryItem_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_InventoryItem_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCk_InventoryItem_Definition> _Definition;

public:
    CK_PROPERTY_GET(_Definition);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_InventoryItem_ParamsData, _Definition);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Fragment_InventoryItem_DimensionsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_InventoryItem_DimensionsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Dimensions = FIntPoint(1, 1);

public:
    CK_PROPERTY_GET(_Dimensions);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_InventoryItem_DimensionsData, _Dimensions);
};

// --------------------------------------------------------------------------------------------------------------------
