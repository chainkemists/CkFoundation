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

#include "CkInventory_DataOnly_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKINVENTORY_API FCk_Fragment_Inventory_DataOnly_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Inventory_DataOnly_ParamsData);

public:
    FCk_Fragment_Inventory_DataOnly_ParamsData() = default;

    explicit FCk_Fragment_Inventory_DataOnly_ParamsData(FGameplayTag InName, TOptional<int32> InBoundLimit = {});

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Inventory"))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Inventory_DataOnly_BoundMode _BoundMode = ECk_Inventory_DataOnly_BoundMode::Unbounded;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1, EditConditionHides,
                      EditCondition = "_BoundMode == ECk_Inventory_DataOnly_BoundMode::Bounded"))
    int32 _BoundLimit = 1;

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
struct CKINVENTORY_API FCk_Fragment_MultipleInventory_DataOnly_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleInventory_DataOnly_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Name"))
    TArray<FCk_Fragment_Inventory_DataOnly_ParamsData> _InventoryParams;

public:
    CK_PROPERTY_GET(_InventoryParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleInventory_DataOnly_ParamsData, _InventoryParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Inventory_DataOnly_BoundsInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Inventory_DataOnly_BoundsInfo);

    FCk_Inventory_DataOnly_BoundsInfo() = default;

    /** Constructs a Bounded result. Use the default constructor for Unbounded. */
    explicit FCk_Inventory_DataOnly_BoundsInfo(int32 InValue)
        : _Mode(ECk_Inventory_DataOnly_BoundMode::Bounded), _Value(InValue) {}

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Inventory_DataOnly_BoundMode _Mode = ECk_Inventory_DataOnly_BoundMode::Unbounded;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides,
                      EditCondition = "_Mode == ECk_Inventory_DataOnly_BoundMode::Bounded"))
    int32 _Value = 0;

public:
    CK_PROPERTY_GET(_Mode);
    CK_PROPERTY_GET(_Value);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_InventoryItem_DataOnly_ReplicatedEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_InventoryItem_DataOnly_ReplicatedEntry);

    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _ItemHandle;

public:
    CK_PROPERTY_GET(_ItemHandle);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_InventoryItem_DataOnly_ReplicatedEntry, _ItemHandle);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKINVENTORY_API FCk_RepData_Inventory_DataOnly_Items
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_Inventory_DataOnly_Items);

    UPROPERTY()
    TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry> Items;
};
