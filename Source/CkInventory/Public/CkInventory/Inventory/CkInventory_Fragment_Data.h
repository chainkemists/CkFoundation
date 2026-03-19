#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTags.h>

#include "CkInventory_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKINVENTORY_API FCk_Handle_Inventory : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Inventory); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Inventory);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_InventoryType : uint8
{
    DataOnly,
    Spatial
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InventoryType);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Fragment_Inventory_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Inventory_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Inventory"))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InventoryType _InventoryType = ECk_InventoryType::DataOnly;

    // Grid dimensions, only used when InventoryType == Spatial
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_InventoryType == ECk_InventoryType::Spatial"))
    FIntPoint _Dimensions = FIntPoint(1, 1);

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_InventoryType);
    CK_PROPERTY_GET(_Dimensions);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Inventory_ParamsData, _Name, _InventoryType);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Request_Inventory_AddItem : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Inventory_AddItem);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Inventory_AddItem);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _ItemToAdd;

    // For spatial inventories: placement coordinate. (-1,-1) means auto-place.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _PlacementCoordinate = FIntPoint(-1, -1);

public:
    CK_PROPERTY(_ItemToAdd);
    CK_PROPERTY(_PlacementCoordinate);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_AddItem, _ItemToAdd);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Request_Inventory_RemoveItem : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Inventory_RemoveItem);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Inventory_RemoveItem);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _ItemToRemove;

public:
    CK_PROPERTY(_ItemToRemove);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_RemoveItem, _ItemToRemove);
};

// --------------------------------------------------------------------------------------------------------------------

// Replicated entry: item handle + spatial placement coordinate
USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_InventoryItem_ReplicatedEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_InventoryItem_ReplicatedEntry);

    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _ItemHandle;

    // Placement coordinate for spatial inventories. (-1,-1) for DataOnly inventories.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Coordinate = FIntPoint(-1, -1);

public:
    CK_PROPERTY(_ItemHandle);
    CK_PROPERTY(_Coordinate);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_InventoryItem_ReplicatedEntry, _ItemHandle, _Coordinate);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Inventory_OnItemsChanged,
    FCk_Handle_Inventory, InInventory,
    const TArray<FCk_Handle>&, InItemsAdded,
    const TArray<FCk_Handle>&, InItemsRemoved);

// --------------------------------------------------------------------------------------------------------------------
