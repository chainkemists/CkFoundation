#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Request/CkRequest_Data.h"

#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"
#include "CkInventory/Item/CkItem_Fragment_Data.h"

#include <GameplayTags.h>

#include "CkInventory_Spatial_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKINVENTORY_API FCk_Fragment_Inventory_Spatial_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Inventory_Spatial_ParamsData);

public:
    FCk_Fragment_Inventory_Spatial_ParamsData() = default;

    explicit FCk_Fragment_Inventory_Spatial_ParamsData(FGameplayTag InName, FIntPoint InDimensions);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Inventory"))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Dimensions = FIntPoint(1, 1);

    FCk_Delegate_Inventory_CustomCanAcceptItem _CustomCanAcceptItem;

    UPROPERTY(BlueprintReadWrite, DisplayName = "Custom Can Accept Item",
              meta = (AllowPrivateAccess = true))
    FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic _CustomCanAcceptItemDynamic;

    UPROPERTY(EditAnywhere, DisplayName = "Custom Can Accept Item",
              meta = (AllowPrivateAccess = true,
                      FunctionReference,
                      AllowFunctionLibraries,
                      PrototypeFunction = "/Script/CkInventory.Ck_Utils_Inventory_UE.Prototype_CanAcceptItem"))
    FMemberReference _CanAcceptItemRef;

    FCk_Delegate_Inventory_CustomCanStackItems _CustomCanStackItems;

    UPROPERTY(BlueprintReadWrite, DisplayName = "Custom Can Stack Items",
              meta = (AllowPrivateAccess = true))
    FCk_Delegate_Inventory_CustomCanStackItems_Dynamic _CustomCanStackItemsDynamic;

    UPROPERTY(EditAnywhere, DisplayName = "Custom Can Stack Items",
              meta = (AllowPrivateAccess = true,
                      FunctionReference,
                      AllowFunctionLibraries,
                      PrototypeFunction = "/Script/CkInventory.Ck_Utils_ItemTrait_Stackable_UE.Prototype_CanStackItems"))
    FMemberReference _CanStackItemsRef;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_Dimensions);
    CK_PROPERTY(_CustomCanAcceptItem);
    CK_PROPERTY(_CustomCanAcceptItemDynamic);
    CK_PROPERTY(_CanAcceptItemRef);
    CK_PROPERTY(_CustomCanStackItems);
    CK_PROPERTY(_CustomCanStackItemsDynamic);
    CK_PROPERTY(_CanStackItemsRef);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Fragment_MultipleInventory_Spatial_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleInventory_Spatial_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Name"))
    TArray<FCk_Fragment_Inventory_Spatial_ParamsData> _InventoryParams;

public:
    CK_PROPERTY_GET(_InventoryParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleInventory_Spatial_ParamsData, _InventoryParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Request_Inventory_Spatial_RelocateItem : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Inventory_Spatial_RelocateItem);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Inventory_Spatial_RelocateItem);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _Item;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_SpatialPlacement _NewPlacement;

public:
    CK_PROPERTY_GET(_Item);
    CK_PROPERTY(_NewPlacement);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Inventory_Spatial_RelocateItem, _Item, _NewPlacement);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_InventoryItem_Spatial_ReplicatedEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_InventoryItem_Spatial_ReplicatedEntry);

    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _ItemHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Coordinate = ck::Inventory::AutoPlaceCoordinate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_CardinalRotation _Rotation = ECk_CardinalRotation::None;

public:
    CK_PROPERTY_GET(_ItemHandle);
    CK_PROPERTY(_Coordinate);
    CK_PROPERTY(_Rotation);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_InventoryItem_Spatial_ReplicatedEntry, _ItemHandle);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKINVENTORY_API FCk_RepData_Inventory_Spatial_Items
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_Inventory_Spatial_Items);

    UPROPERTY()
    TArray<FCk_InventoryItem_Spatial_ReplicatedEntry> Items;
};
