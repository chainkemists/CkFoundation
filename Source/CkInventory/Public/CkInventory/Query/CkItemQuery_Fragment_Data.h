#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkItemQuery_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_ItemTrait;
class UCk_InventoryItem_Definition;

// --------------------------------------------------------------------------------------------------------------------

// Optional per-definition predicate, applied on top of the trait filters. Two
// flavours (mirrors CkInventory's CustomCanAcceptItem): a native C++ delegate and
// a BP/AS-bindable dynamic delegate. Dynamic delegates can't return a value, so
// it reports through a bool& out-param. A bound predicate must pass for the
// definition to be included; both (if bound) must pass.
DECLARE_DELEGATE_RetVal_OneParam(
    bool,
    FCk_Delegate_ItemQuery_CustomFilter,
    const UCk_InventoryItem_Definition*);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ItemQuery_CustomFilter_Dynamic,
    UCk_InventoryItem_Definition*, InDefinition,
    bool&, OutPasses);

// --------------------------------------------------------------------------------------------------------------------

// Declarative trait filter over item definitions. Empty arrays impose no
// constraint on that axis. A definition matches when it has ALL of RequiredAll,
// at least one of RequiredAny, and NONE of Excluded — and passes any bound
// custom predicate(s).
USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_ItemQuery_Filter
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ItemQuery_Filter);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<TSubclassOf<UCk_ItemTrait>> _RequiredAll;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<TSubclassOf<UCk_ItemTrait>> _RequiredAny;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<TSubclassOf<UCk_ItemTrait>> _Excluded;

    // Native C++-only predicate (not reflected — settable from C++ callers).
    FCk_Delegate_ItemQuery_CustomFilter _CustomFilter;

    // BP/AS-bindable predicate. Reports inclusion via the OutPasses out-param.
    UPROPERTY(BlueprintReadWrite, DisplayName = "Custom Filter",
              meta = (AllowPrivateAccess = true))
    FCk_Delegate_ItemQuery_CustomFilter_Dynamic _CustomFilterDynamic;

public:
    CK_PROPERTY(_RequiredAll);
    CK_PROPERTY(_RequiredAny);
    CK_PROPERTY(_Excluded);
    CK_PROPERTY(_CustomFilter);
    CK_PROPERTY(_CustomFilterDynamic);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Request_ItemQuery_QueryDefinitions : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ItemQuery_QueryDefinitions);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_ItemQuery_QueryDefinitions);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ItemQuery_Filter _Filter;

public:
    CK_PROPERTY(_Filter);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_ItemQuery_QueryDefinitions, _Filter);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_ItemQuery_Result
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ItemQuery_Result);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<TObjectPtr<UCk_InventoryItem_Definition>> _Definitions;

public:
    CK_PROPERTY(_Definitions);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_ItemQuery_OnQueried,
    const FCk_ItemQuery_Result&, InResult);

// --------------------------------------------------------------------------------------------------------------------
