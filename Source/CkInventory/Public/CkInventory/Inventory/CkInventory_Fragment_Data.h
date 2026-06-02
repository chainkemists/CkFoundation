#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Enums/CkEnums.h"
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

    /** Sentinel for stack-count parameters meaning "all available" (e.g. transfer the full source
     *  stack, stack the full source onto the target). Negative-by-design so a positive count is
     *  always interpreted literally. */
    inline constexpr int32 AllAvailableCount = -1;
}

// Gameplay tag for the internal bound max integer attribute on DataOnly inventories.
CKINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IntegerAttribute_InventoryBoundMax);

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

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
enum class ECk_ItemResolution_StackingPreference : uint8
{
    // Existing partial stack of the same definition wins over emptier inventories.
    // Falls back to capacity-based ranking when no stackable target exists.
    Prefer,
    // Only return a candidate that can merge into an existing stack. Invalid handle otherwise.
    Require,
    // Ignore stacking. Rank purely by remaining capacity (free slots / cells).
    Ignore
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ItemResolution_StackingPreference);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Inventory_OperationResult_Add : uint8
{
    Success,
    Failed_InvalidItem,
    Failed_ItemAlreadyInInventory,
    Failed_NoSpaceAvailable,
    Failed_PlacementBlocked,
    Failed_RejectedByCustomAcceptanceLogic,
    Failed_MissingDimensionsTrait
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

UENUM(BlueprintType)
enum class ECk_Inventory_OperationResult_Relocate : uint8
{
    Success,
    Failed_InvalidItem,
    Failed_ItemNotInInventory,
    Failed_NotSpatialInventory,
    Failed_PlacementBlocked
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Inventory_OperationResult_Relocate);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DELEGATE_RetVal_TwoParams(
    bool,
    FCk_Delegate_Inventory_CustomCanAcceptItem,
    FCk_Handle_Inventory,
    FCk_Handle_Item);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic,
    FCk_Handle_Inventory, InInventory,
    FCk_Handle_Item, InItem,
    bool&, OutCanAccept);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DELEGATE_RetVal_ThreeParams(
    bool,
    FCk_Delegate_Inventory_CustomCanStackItems,
    FCk_Handle_Inventory,
    FCk_Handle_Item,
    FCk_Handle_Item);

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_Inventory_CustomCanStackItems_Dynamic,
    FCk_Handle_Inventory, InInventory,
    FCk_Handle_Item, InSourceItem,
    FCk_Handle_Item, InTargetItem,
    bool&, OutCanStack);

// --------------------------------------------------------------------------------------------------------------------

// Sort comparator for inventory candidates: returns true if InCandidateA should rank ahead of InCandidateB
// when picking a transfer target for InItem. Same shape as a TArray::Sort predicate.
DECLARE_DELEGATE_RetVal_ThreeParams(
    bool,
    FCk_Delegate_ItemResolution_CustomSort,
    FCk_Handle_Inventory,
    FCk_Handle_Inventory,
    FCk_Handle_Item);

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_ItemResolution_CustomSort_Dynamic,
    FCk_Handle_Inventory, InCandidateA,
    FCk_Handle_Inventory, InCandidateB,
    FCk_Handle_Item, InItem,
    bool&, OutAIsBetter);

// --------------------------------------------------------------------------------------------------------------------
// Reusable {coordinate, rotation} pair for spatial placement. Used by AddItem/SplitStack as the
// addon carrier on Spatial inventories, and embedded in TransferItem_ToSpatial / RelocateItem
// requests. Default-constructed = AutoPlace + None rotation ("let the system pick").
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_SpatialPlacement
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SpatialPlacement);

    FCk_SpatialPlacement() = default;

    FCk_SpatialPlacement(FIntPoint InCoordinate, ECk_CardinalRotation InRotation)
        : _Coordinate(InCoordinate), _Rotation(InRotation) {}

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Coordinate = ck::Inventory::AutoPlaceCoordinate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_CardinalRotation _Rotation = ECk_CardinalRotation::None;

public:
    CK_PROPERTY(_Coordinate);
    CK_PROPERTY(_Rotation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_SpatialPlacementResult
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SpatialPlacementResult);

    static auto
    Success(FIntPoint InCoordinate, ECk_CardinalRotation InRotation) -> FCk_SpatialPlacementResult;

    static auto
    Failed() -> FCk_SpatialPlacementResult;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _Succeeded = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Coordinate = ck::Inventory::AutoPlaceCoordinate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_CardinalRotation _Rotation = ECk_CardinalRotation::None;

public:
    CK_PROPERTY(_Succeeded);
    CK_PROPERTY(_Coordinate);
    CK_PROPERTY(_Rotation);
};

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Fragment_Inventory_DataOnly_ParamsData;
struct FCk_Fragment_Inventory_Spatial_ParamsData;

USTRUCT()
struct CKINVENTORY_API FCk_Fragment_Inventory_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Inventory_ParamsData);
    // Tier-A marker. Note: all fields are non-UPROPERTY (internal copy struct); zero fields are SaveGame-tagged.
    // The typed Spatial/DataOnly ParamsData structs are the real user-facing save targets.
    using IsSnapshotable = void;

public:
    FCk_Fragment_Inventory_ParamsData() = default;

    /** DataOnly inventory — copies the shared field set + bound mode/limit from the typed params. */
    explicit FCk_Fragment_Inventory_ParamsData(const FCk_Fragment_Inventory_DataOnly_ParamsData& InDataOnlyParams);

    /** Spatial inventory — copies the shared field set + grid dimensions from the typed params. */
    explicit FCk_Fragment_Inventory_ParamsData(const FCk_Fragment_Inventory_Spatial_ParamsData& InSpatialParams);

private:
    FGameplayTag _Name = FGameplayTag::EmptyTag;
    ECk_InventoryType _InventoryType = ECk_InventoryType::DataOnly;
    FIntPoint _Dimensions = FIntPoint(1, 1);
    ECk_Inventory_DataOnly_BoundMode _BoundMode = ECk_Inventory_DataOnly_BoundMode::Unbounded;
    int32 _BoundLimit = 1;
    FCk_Delegate_Inventory_CustomCanAcceptItem _CustomCanAcceptItem;
    FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic _CustomCanAcceptItemDynamic;
    FMemberReference _CanAcceptItemRef;
    FCk_Delegate_Inventory_CustomCanStackItems _CustomCanStackItems;
    FCk_Delegate_Inventory_CustomCanStackItems_Dynamic _CustomCanStackItemsDynamic;
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

DECLARE_DELEGATE_RetVal_TwoParams(
    bool,
    FCk_Delegate_Inventory_SortPredicate,
    FCk_Handle_Item,
    FCk_Handle_Item);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Inventory_SortPredicate_Dynamic,
    FCk_Handle_Item, InItemA,
    FCk_Handle_Item, InItemB,
    bool&, OutABeforeB);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_BestTransferTargetParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_BestTransferTargetParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle_Inventory> _Candidates;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ItemResolution_StackingPreference _StackingPreference = ECk_ItemResolution_StackingPreference::Prefer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Inventory_AddPolicy _AddPolicy = ECk_Inventory_AddPolicy::PreferStacking;

    // Native C++ sort comparator. When bound, replaces the built-in policy comparator.
    FCk_Delegate_ItemResolution_CustomSort _CustomSort;

    // Blueprint sort comparator. When bound (and _CustomSort is not), replaces the built-in policy comparator.
    UPROPERTY(BlueprintReadWrite, DisplayName = "Custom Sort",
              meta = (AllowPrivateAccess = true))
    FCk_Delegate_ItemResolution_CustomSort_Dynamic _CustomSortDynamic;

public:
    CK_PROPERTY_GET(_Candidates);
    CK_PROPERTY(_StackingPreference);
    CK_PROPERTY(_AddPolicy);
    CK_PROPERTY(_CustomSort);
    CK_PROPERTY(_CustomSortDynamic);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_BestTransferTargetParams, _Candidates);
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

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_Inventory_OnOperationResult_AddByDefinition,
    FCk_Handle_Inventory, InInventory,
    ECk_Inventory_OperationResult_AddByDefinition, InResult,
    int32, InAmountAdded,
    const TArray<FCk_Handle_Item>&, InItemsCreated);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_SixParams(
    FCk_Delegate_Inventory_OnOperationResult_Transfer,
    FCk_Handle_Inventory, InSourceInventory,
    FCk_Handle_Item, InItem,
    FCk_Handle_Inventory, InTargetInventory,
    int32, InCountTransferred,
    FCk_Handle_Item, InNewItemInTarget,
    ECk_Inventory_OperationResult_Transfer, InResult);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Inventory_OnOperationResult_Sort,
    FCk_Handle_Inventory, InInventory,
    ECk_Inventory_OperationResult_Sort, InResult);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_Inventory_OnOperationResult_Relocate,
    FCk_Handle_Inventory, InInventory,
    FCk_Handle_Item, InItem,
    FIntPoint, InNewCoordinate,
    ECk_Inventory_OperationResult_Relocate, InResult);

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

public:
    CK_PROPERTY_GET(_ItemToAdd);

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Count = ck::Inventory::AllAvailableCount;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1))
    int32 _SplitCount = 1;

public:
    CK_PROPERTY_GET(_SourceItem);
    CK_PROPERTY_GET(_SplitCount);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_SplitStack, _SourceItem, _SplitCount);
};

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;

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
    const UCk_InventoryItem_Definition* _Definition = nullptr;

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
struct CKINVENTORY_API FCk_Request_Inventory_Sort : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Inventory_Sort);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Inventory_Sort);

private:
    FCk_Delegate_Inventory_SortPredicate _SortPredicate;

    UPROPERTY(BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Delegate_Inventory_SortPredicate_Dynamic _SortPredicateDynamic;

public:
    CK_PROPERTY(_SortPredicate);
    CK_PROPERTY(_SortPredicateDynamic);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Request_Inventory_TransferItem_ToSpatial : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Inventory_TransferItem_ToSpatial);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Inventory_TransferItem_ToSpatial);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _SourceItem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Inventory_Spatial _TargetInventory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Count = ck::Inventory::AllAvailableCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Inventory_AddPolicy _Policy = ECk_Inventory_AddPolicy::PreferStacking;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_SpatialPlacement _Placement;

public:
    CK_PROPERTY_GET(_SourceItem);
    CK_PROPERTY_GET(_TargetInventory);
    CK_PROPERTY(_Count);
    CK_PROPERTY(_Policy);
    CK_PROPERTY(_Placement);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_TransferItem_ToSpatial, _SourceItem, _TargetInventory);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Request_Inventory_TransferItem_ToDataOnly : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Inventory_TransferItem_ToDataOnly);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Inventory_TransferItem_ToDataOnly);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _SourceItem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Inventory_DataOnly _TargetInventory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Count = ck::Inventory::AllAvailableCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Inventory_AddPolicy _Policy = ECk_Inventory_AddPolicy::PreferStacking;

public:
    CK_PROPERTY_GET(_SourceItem);
    CK_PROPERTY_GET(_TargetInventory);
    CK_PROPERTY(_Count);
    CK_PROPERTY(_Policy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_TransferItem_ToDataOnly, _SourceItem, _TargetInventory);
};

// --------------------------------------------------------------------------------------------------------------------
