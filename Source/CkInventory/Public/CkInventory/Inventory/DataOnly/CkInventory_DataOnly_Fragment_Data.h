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

    explicit FCk_Fragment_Inventory_DataOnly_ParamsData(
        FGameplayTag InName,
        TOptional<int32> InBoundLimit = {},
        ECk_Inventory_DataOnly_BoundMode InBoundMode = ECk_Inventory_DataOnly_BoundMode::BoundedByUniqueEntries);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Inventory", SaveGame))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, SaveGame))
    ECk_Inventory_DataOnly_BoundMode _BoundMode = ECk_Inventory_DataOnly_BoundMode::Unbounded;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1, EditConditionHides,
                      EditCondition = "_BoundMode != ECk_Inventory_DataOnly_BoundMode::Unbounded", SaveGame))
    int32 _BoundLimit = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, SaveGame))
    ECk_Inventory_StackingPolicy _StackingPolicy = ECk_Inventory_StackingPolicy::UseItemDefinition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1, EditConditionHides,
                      EditCondition = "_StackingPolicy == ECk_Inventory_StackingPolicy::ClampMaxStackSize", SaveGame))
    int32 _MaxStackSizeClamp = 1;

    // Not SaveGame: native delegate — runtime wiring, not persisted state.
    FCk_Delegate_Inventory_CustomCanAcceptItem _CustomCanAcceptItem;

    // Not SaveGame: dynamic delegate — runtime wiring, not persisted state.
    UPROPERTY(BlueprintReadWrite, DisplayName = "Custom Can Accept Item",
              meta = (AllowPrivateAccess = true))
    FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic _CustomCanAcceptItemDynamic;

    // Not SaveGame: FMemberReference — Blueprint function reference, not persisted state.
    UPROPERTY(EditAnywhere, DisplayName = "Custom Can Accept Item",
              meta = (AllowPrivateAccess = true,
                      FunctionReference,
                      AllowFunctionLibraries,
                      PrototypeFunction = "/Script/CkInventory.Ck_Utils_Inventory_UE.Prototype_CanAcceptItem"))
    FMemberReference _CanAcceptItemRef;

    // Not SaveGame: native delegate — runtime wiring, not persisted state.
    FCk_Delegate_Inventory_CustomCanStackItems _CustomCanStackItems;

    // Not SaveGame: dynamic delegate — runtime wiring, not persisted state.
    UPROPERTY(BlueprintReadWrite, DisplayName = "Custom Can Stack Items",
              meta = (AllowPrivateAccess = true))
    FCk_Delegate_Inventory_CustomCanStackItems_Dynamic _CustomCanStackItemsDynamic;

    // Not SaveGame: FMemberReference — Blueprint function reference, not persisted state.
    UPROPERTY(EditAnywhere, DisplayName = "Custom Can Stack Items",
              meta = (AllowPrivateAccess = true,
                      FunctionReference,
                      AllowFunctionLibraries,
                      PrototypeFunction = "/Script/CkInventory.Ck_Utils_ItemTrait_Stackable_UE.Prototype_CanStackItems"))
    FMemberReference _CanStackItemsRef;

    // Not SaveGame: native delegate — runtime wiring, not persisted state.
    FCk_Delegate_Inventory_CustomGetAbsorbableUnits _CustomGetAbsorbableUnits;

    // Not SaveGame: dynamic delegate — runtime wiring, not persisted state.
    UPROPERTY(BlueprintReadWrite, DisplayName = "Custom Get Absorbable Units",
              meta = (AllowPrivateAccess = true))
    FCk_Delegate_Inventory_CustomGetAbsorbableUnits_Dynamic _CustomGetAbsorbableUnitsDynamic;

    // Not SaveGame: FMemberReference — Blueprint function reference, not persisted state.
    UPROPERTY(EditAnywhere, DisplayName = "Custom Get Absorbable Units",
              meta = (AllowPrivateAccess = true,
                      FunctionReference,
                      AllowFunctionLibraries,
                      PrototypeFunction = "/Script/CkInventory.Ck_Utils_Inventory_UE.Prototype_GetAbsorbableUnits"))
    FMemberReference _GetAbsorbableUnitsRef;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_BoundMode);
    CK_PROPERTY_GET(_BoundLimit);
    CK_PROPERTY(_StackingPolicy);
    CK_PROPERTY(_MaxStackSizeClamp);
    CK_PROPERTY(_CustomCanAcceptItem);
    CK_PROPERTY(_CustomCanAcceptItemDynamic);
    CK_PROPERTY(_CanAcceptItemRef);
    CK_PROPERTY(_CustomCanStackItems);
    CK_PROPERTY(_CustomCanStackItemsDynamic);
    CK_PROPERTY(_CanStackItemsRef);
    CK_PROPERTY(_CustomGetAbsorbableUnits);
    CK_PROPERTY(_CustomGetAbsorbableUnitsDynamic);
    CK_PROPERTY(_GetAbsorbableUnitsRef);
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

    /** Constructs a Bounded result (mode names the metric the value limits).
     *  Use the default constructor for Unbounded. */
    explicit FCk_Inventory_DataOnly_BoundsInfo(ECk_Inventory_DataOnly_BoundMode InMode, int32 InValue)
        : _Mode(InMode), _Value(InValue) {}

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Inventory_DataOnly_BoundMode _Mode = ECk_Inventory_DataOnly_BoundMode::Unbounded;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides,
                      EditCondition = "_Mode != ECk_Inventory_DataOnly_BoundMode::Unbounded"))
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
              meta = (AllowPrivateAccess = true, SaveGame))
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
