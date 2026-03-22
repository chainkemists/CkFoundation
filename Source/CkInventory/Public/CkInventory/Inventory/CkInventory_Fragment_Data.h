#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Request/CkRequest_Data.h"

#include "CkInventory/Item/CkInventoryItem_Fragment_Data.h"

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

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKINVENTORY_API FCk_Fragment_Inventory_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Inventory_ParamsData);

public:
    FCk_Fragment_Inventory_ParamsData() = default;
    /** DataOnly inventory */
    explicit FCk_Fragment_Inventory_ParamsData(FGameplayTag InName);

    /** Spatial inventory */
    explicit FCk_Fragment_Inventory_ParamsData(FGameplayTag InName, FIntPoint InDimensions);

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
    FCk_Handle_Item _ItemToAdd;

    // For spatial inventories: placement coordinate. (-1,-1) means auto-place.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _PlacementCoordinate = FIntPoint(-1, -1);

public:
    CK_PROPERTY_GET(_ItemToAdd);
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
    FCk_Handle_Item _ItemToRemove;

public:
    CK_PROPERTY_GET(_ItemToRemove);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_RemoveItem, _ItemToRemove);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Request_Inventory_StackItems : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Inventory_StackItems);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Inventory_StackItems);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _SourceItem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _TargetItem;

    // How many to transfer from source to target. -1 means "as many as possible".
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Count = -1;

public:
    CK_PROPERTY_GET(_SourceItem);
    CK_PROPERTY_GET(_TargetItem);
    CK_PROPERTY_GET(_Count);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_StackItems, _SourceItem, _TargetItem);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Request_Inventory_SplitStack : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Inventory_SplitStack);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Inventory_SplitStack);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _SourceItem;

    // How many to split off into the new item.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1))
    int32 _SplitCount = 1;

    // For spatial inventories: placement coordinate for the new item. (-1,-1) means auto-place.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _PlacementCoordinate = FIntPoint(-1, -1);

public:
    CK_PROPERTY_GET(_SourceItem);
    CK_PROPERTY_GET(_SplitCount);
    CK_PROPERTY(_PlacementCoordinate);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_SplitStack, _SourceItem, _SplitCount);
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Inventory_OperationResult_Add : uint8
{
    Success,
    Failed_InvalidItem,
    Failed_ItemAlreadyInInventory,
    Failed_NoSpaceAvailable,
    Failed_PlacementBlocked
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Inventory_OperationResult_Add);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Inventory_OperationResult_Remove : uint8
{
    Success,
    Failed_InvalidItem,
    Failed_ItemNotInInventory
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Inventory_OperationResult_Remove);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Inventory_OperationResult_Stack : uint8
{
    Success,
    Failed_InvalidSourceItem,
    Failed_InvalidTargetItem,
    Failed_SourceNotInInventory,
    Failed_TargetNotInInventory,
    Failed_ItemsNotStackable,
    Failed_DefinitionMismatch,
    Failed_IncompatibleFragments,
    Failed_TargetStackFull
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Inventory_OperationResult_Stack);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Inventory_OperationResult_Split : uint8
{
    Success,
    Failed_InvalidSourceItem,
    Failed_SourceNotInInventory,
    Failed_ItemNotStackable,
    Failed_InsufficientCount,
    Failed_NoSpaceForNewItem
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Inventory_OperationResult_Split);

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
    FCk_Handle_Item _ItemHandle;

    // Placement coordinate for spatial inventories. (-1,-1) for DataOnly inventories.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Coordinate = FIntPoint(-1, -1);

public:
    CK_PROPERTY_GET(_ItemHandle);
    CK_PROPERTY(_Coordinate);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_InventoryItem_ReplicatedEntry, _ItemHandle);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Inventory_OnItemsChanged,
    FCk_Handle_Inventory, InInventory,
    const TArray<FCk_Handle_Item>&, InItemsAdded,
    const TArray<FCk_Handle_Item>&, InItemsRemoved);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Inventory_OnOperationResult_Add,
    FCk_Handle_Inventory, InInventory,
    FCk_Handle_Item, InItem,
    ECk_Inventory_OperationResult_Add, InResult);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Inventory_OnOperationResult_Remove,
    FCk_Handle_Inventory, InInventory,
    FCk_Handle_Item, InItem,
    ECk_Inventory_OperationResult_Remove, InResult);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_Inventory_OnOperationResult_Stack,
    FCk_Handle_Inventory, InInventory,
    FCk_Handle_Item, InSourceItem,
    FCk_Handle_Item, InTargetItem,
    ECk_Inventory_OperationResult_Stack, InResult);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_Inventory_OnOperationResult_Split,
    FCk_Handle_Inventory, InInventory,
    FCk_Handle_Item, InSourceItem,
    FCk_Handle_Item, InNewItem,
    ECk_Inventory_OperationResult_Split, InResult);

// --------------------------------------------------------------------------------------------------------------------
