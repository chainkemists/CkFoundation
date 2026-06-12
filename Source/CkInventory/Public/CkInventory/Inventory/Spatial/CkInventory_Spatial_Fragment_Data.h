#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Request/CkRequest_Data.h"

#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"
#include "CkInventory/Item/CkItem_Fragment_Data.h"

#include "CkEcs/Concepts/CkSnapshot_Concepts.h" // forward-declares ck::FSnapshotContext for the SerializeSnapshot decl

#include <GameplayTags.h>

#include "CkInventory_Spatial_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;
class FArchive;

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKINVENTORY_API FCk_Fragment_Inventory_Spatial_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Inventory_Spatial_ParamsData);
    using IsSnapshotable = void;

public:
    FCk_Fragment_Inventory_Spatial_ParamsData() = default;

    explicit FCk_Fragment_Inventory_Spatial_ParamsData(FGameplayTag InName, FIntPoint InDimensions);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Inventory", SaveGame))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, SaveGame))
    FIntPoint _Dimensions = FIntPoint(1, 1);

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
    CK_PROPERTY_GET(_Dimensions);
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
    // Tier-B: carries FCk_Handle_Item (entity ref) — _ItemHandle is remapped via FSnapshotContext.
    using IsSnapshotable = void;

    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

    // Tier-B: _ItemHandle remapped via FSnapshotContext; _Coordinate / _Rotation are plain. Body in the .cpp.
    auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx) -> void;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, SaveGame))
    FCk_Handle_Item _ItemHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, SaveGame))
    FIntPoint _Coordinate = ck::Inventory::AutoPlaceCoordinate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, SaveGame))
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
