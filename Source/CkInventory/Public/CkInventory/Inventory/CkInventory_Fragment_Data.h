#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Request/CkRequest_Data.h"

#include "CkInventory/Item/CkItem_Fragment_Data.h"

#include <GameplayTags.h>
#include <NativeGameplayTags.h>
#include <Engine/BlueprintGeneratedClass.h>

#include "CkInventory_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::Inventory
{
    /** Sentinel value indicating auto-placement for spatial inventories. */
    inline const FIntPoint AutoPlaceCoordinate{-1, -1};

    /** Sentinel value indicating unbounded capacity for DataOnly inventories. */
    inline constexpr int32 UnboundedBoundLimit = -1;
}

// Gameplay tag for the internal bound max integer attribute on DataOnly inventories.
CKINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IntegerAttribute_InventoryBoundMax);

// ============================================================================
// Handles
// ============================================================================

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKINVENTORY_API FCk_Handle_Inventory : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Inventory);
    friend struct FCk_Handle_Inventory_Spatial;
    friend struct FCk_Handle_Inventory_DataOnly;
};
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Inventory);

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKINVENTORY_API FCk_Handle_Inventory_Spatial : public FCk_Handle_Inventory { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_DERIVED(FCk_Handle_Inventory_Spatial, FCk_Handle_Inventory); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Inventory_Spatial);

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKINVENTORY_API FCk_Handle_Inventory_DataOnly : public FCk_Handle_Inventory { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_DERIVED(FCk_Handle_Inventory_DataOnly, FCk_Handle_Inventory); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Inventory_DataOnly);

// ============================================================================
// Enums
// ============================================================================

UENUM(BlueprintType)
enum class ECk_InventoryType : uint8
{
    DataOnly,
    Spatial
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InventoryType);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Inventory_DataOnly_BoundMode : uint8
{
    Unbounded,
    Bounded
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Inventory_DataOnly_BoundMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Inventory_AddPolicy : uint8
{
    // Fill existing compatible stacks first, then create new items for the remainder
    PreferStacking,
    // Always create fresh items, never merge into existing stacks
    ForceNewItem
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Inventory_AddPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Inventory_OperationResult_Add : uint8
{
    Success,
    Failed_InvalidItem,
    Failed_ItemAlreadyInInventory,
    Failed_NoSpaceAvailable,
    Failed_PlacementBlocked,
    Failed_RejectedByCustomAcceptanceLogic
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
    Failed_TargetStackFull,
    Failed_RejectedByCustomStackLogic
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

UENUM(BlueprintType)
enum class ECk_Inventory_OperationResult_AddByDefinition : uint8
{
    Success_AllAdded,
    Success_PartiallyAdded,
    Failed_InvalidDefinition,
    Failed_NoSpaceAvailable,
    Failed_ZeroAmount,
    Failed_RejectedByCustomAcceptanceLogic
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Inventory_OperationResult_AddByDefinition);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Inventory_OperationResult_Transfer : uint8
{
    Success,
    Success_Partial,
    Failed_InvalidSourceItem,
    Failed_SourceNotInInventory,
    Failed_InvalidTargetInventory,
    Failed_SameInventory,
    Failed_NoSpaceInTarget,
    Failed_ZeroCount,
    Failed_IncompatibleFragments,
    Failed_RejectedByCustomAcceptanceLogic
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Inventory_OperationResult_Transfer);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Inventory_OperationResult_Sort : uint8
{
    Success,
    Failed_InvalidInventory
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Inventory_OperationResult_Sort);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DELEGATE_RetVal_TwoParams(
    bool,
    FCk_Delegate_Inventory_CustomCanAcceptItem,
    FCk_Handle_Inventory, /* InInventory */
    FCk_Handle_Item       /* InItem */);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic,
    FCk_Handle_Inventory, InInventory,
    FCk_Handle_Item, InItem,
    bool&, OutCanAccept);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DELEGATE_RetVal_ThreeParams(
    bool,
    FCk_Delegate_Inventory_CustomCanStackItems,
    FCk_Handle_Inventory, /* InInventory */
    FCk_Handle_Item,      /* InSourceItem */
    FCk_Handle_Item       /* InTargetItem */);

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_Inventory_CustomCanStackItems_Dynamic,
    FCk_Handle_Inventory, InInventory,
    FCk_Handle_Item, InSourceItem,
    FCk_Handle_Item, InTargetItem,
    bool&, OutCanStack);

// ============================================================================
// Structs
// ============================================================================

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKINVENTORY_API FCk_Fragment_Inventory_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Inventory_ParamsData);

public:
    FCk_Fragment_Inventory_ParamsData() = default;

    /** DataOnly inventory (unbounded or bounded if InBoundLimit is set) */
    explicit FCk_Fragment_Inventory_ParamsData(FGameplayTag InName, TOptional<int32> InBoundLimit = {});

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
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_InventoryType == ECk_InventoryType::Spatial"))
    FIntPoint _Dimensions = FIntPoint(1, 1);

    // Bound mode for DataOnly inventories
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides,
                      EditCondition = "_InventoryType == ECk_InventoryType::DataOnly"))
    ECk_Inventory_DataOnly_BoundMode _BoundMode = ECk_Inventory_DataOnly_BoundMode::Unbounded;

    // Maximum number of items for bounded DataOnly inventories (min 1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1, EditConditionHides,
                      EditCondition = "_InventoryType == ECk_InventoryType::DataOnly && _BoundMode == ECk_Inventory_DataOnly_BoundMode::Bounded"))
    int32 _BoundLimit = 1;

    // C++ native callback for item acceptance validation.
    FCk_Delegate_Inventory_CustomCanAcceptItem _CustomCanAcceptItem;

    // Blueprint runtime callback for item acceptance validation.
    // Bind in Blueprint graph via Create Event / Assign nodes.
    UPROPERTY(BlueprintReadWrite, DisplayName = "Custom Can Accept Item",
              meta = (AllowPrivateAccess = true))
    FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic _CustomCanAcceptItemDynamic;

    // Function picker for item acceptance validation.
    // Bind via the Details panel dropdown on persistent objects (actors, data assets).
    UPROPERTY(EditAnywhere, DisplayName = "Custom Can Accept Item",
              meta = (AllowPrivateAccess = true,
                      FunctionReference,
                      AllowFunctionLibraries,
                      PrototypeFunction = "/Script/CkInventory.Ck_Utils_Inventory_UE.Prototype_CanAcceptItem"))
    FMemberReference _CanAcceptItemRef;

    // C++ native callback for stack validation.
    FCk_Delegate_Inventory_CustomCanStackItems _CustomCanStackItems;

    // Blueprint runtime callback for stack validation.
    // Bind in Blueprint graph via Create Event / Assign nodes.
    UPROPERTY(BlueprintReadWrite, DisplayName = "Custom Can Stack Items",
              meta = (AllowPrivateAccess = true))
    FCk_Delegate_Inventory_CustomCanStackItems_Dynamic _CustomCanStackItemsDynamic;

    // Function picker for stack validation.
    // Bind via the Details panel dropdown on persistent objects (actors, data assets).
    UPROPERTY(EditAnywhere, DisplayName = "Custom Can Stack Items",
              meta = (AllowPrivateAccess = true,
                      FunctionReference,
                      AllowFunctionLibraries,
                      PrototypeFunction = "/Script/CkInventory.Ck_Utils_ItemTrait_Stackable_UE.Prototype_CanStackItems"))
    FMemberReference _CanStackItemsRef;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_InventoryType);
    CK_PROPERTY_GET(_Dimensions);
    CK_PROPERTY_GET(_BoundMode);
    CK_PROPERTY_GET(_BoundLimit);
    CK_PROPERTY(_CustomCanAcceptItem);
    CK_PROPERTY(_CustomCanAcceptItemDynamic);
    CK_PROPERTY(_CanAcceptItemRef);
    CK_PROPERTY(_CustomCanStackItems);
    CK_PROPERTY(_CustomCanStackItemsDynamic);
    CK_PROPERTY(_CanStackItemsRef);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Fragment_MultipleInventory_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleInventory_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Name"))
    TArray<FCk_Fragment_Inventory_ParamsData> _InventoryParams;

public:
    CK_PROPERTY_GET(_InventoryParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleInventory_ParamsData, _InventoryParams);
};

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;

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
    FIntPoint _PlacementCoordinate = ck::Inventory::AutoPlaceCoordinate;

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
    FIntPoint _PlacementCoordinate = ck::Inventory::AutoPlaceCoordinate;

public:
    CK_PROPERTY_GET(_SourceItem);
    CK_PROPERTY_GET(_SplitCount);
    CK_PROPERTY(_PlacementCoordinate);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_SplitStack, _SourceItem, _SplitCount);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Request_Inventory_AddItemByDefinition : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Inventory_AddItemByDefinition);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Inventory_AddItemByDefinition);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_InventoryItem_Definition> _Definition = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1))
    int32 _Amount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Inventory_AddPolicy _Policy = ECk_Inventory_AddPolicy::PreferStacking;

public:
    CK_PROPERTY_GET(_Definition);
    CK_PROPERTY_GET(_Amount);
    CK_PROPERTY(_Policy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_AddItemByDefinition, _Definition, _Amount);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Request_Inventory_TransferItem : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Inventory_TransferItem);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Inventory_TransferItem);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _SourceItem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Inventory _TargetInventory;

    // How many to transfer. -1 means "all".
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Count = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Inventory_AddPolicy _Policy = ECk_Inventory_AddPolicy::PreferStacking;

    // For spatial target inventories: placement coordinate. (-1,-1) means auto-place.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _PlacementCoordinate = ck::Inventory::AutoPlaceCoordinate;

public:
    CK_PROPERTY_GET(_SourceItem);
    CK_PROPERTY_GET(_TargetInventory);
    CK_PROPERTY_GET(_Count);
    CK_PROPERTY(_Policy);
    CK_PROPERTY(_PlacementCoordinate);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_TransferItem, _SourceItem, _TargetInventory);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Request_Inventory_SwapItems : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Inventory_SwapItems);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Inventory_SwapItems);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _SourceItem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _TargetItem;

public:
    CK_PROPERTY_GET(_SourceItem);
    CK_PROPERTY_GET(_TargetItem);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_SwapItems, _SourceItem, _TargetItem);
};

// --------------------------------------------------------------------------------------------------------------------

// Sort predicate: returns true if A should come before B
DECLARE_DELEGATE_RetVal_TwoParams(
    bool,
    FCk_Delegate_Inventory_SortPredicate,
    FCk_Handle_Item, /* InItemA */
    FCk_Handle_Item  /* InItemB */);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Inventory_SortPredicate_Dynamic,
    FCk_Handle_Item, InItemA,
    FCk_Handle_Item, InItemB,
    bool&, OutABeforeB);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Request_Inventory_Sort : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Inventory_Sort);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Inventory_Sort);

private:
    // Native C++ sort predicate
    FCk_Delegate_Inventory_SortPredicate _SortPredicate;

    // Blueprint sort predicate
    UPROPERTY(BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Delegate_Inventory_SortPredicate_Dynamic _SortPredicateDynamic;

public:
    CK_PROPERTY(_SortPredicate);
    CK_PROPERTY(_SortPredicateDynamic);
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
    FCk_Handle_Item _ItemHandle;

    // Placement coordinate for spatial inventories. (-1,-1) for DataOnly inventories.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Coordinate = ck::Inventory::AutoPlaceCoordinate;

public:
    CK_PROPERTY_GET(_ItemHandle);
    CK_PROPERTY(_Coordinate);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_InventoryItem_ReplicatedEntry, _ItemHandle);
};

// ============================================================================
// Delegates
// ============================================================================

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

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_Inventory_OnOperationResult_AddByDefinition,
    FCk_Handle_Inventory, InInventory,
    ECk_Inventory_OperationResult_AddByDefinition, InResult,
    int32, InAmountAdded,
    const TArray<FCk_Handle_Item>&, InItemsCreated);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_FiveParams(
    FCk_Delegate_Inventory_OnOperationResult_Transfer,
    FCk_Handle_Inventory, InSourceInventory,
    FCk_Handle_Item, InItem,
    FCk_Handle_Inventory, InTargetInventory,
    int32, InCountTransferred,
    ECk_Inventory_OperationResult_Transfer, InResult);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Inventory_OnOperationResult_Sort,
    FCk_Handle_Inventory, InInventory,
    ECk_Inventory_OperationResult_Sort, InResult);

// --------------------------------------------------------------------------------------------------------------------
