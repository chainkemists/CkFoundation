#pragma once

#include "CkRecord/Record/CkRecord_Fragment_Params.h"

#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/CkEcs_Utils.h"

#include "CkRecord_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKRECORD_API UCk_Utils_Record_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Record_UE);

    friend class UCk_Utils_RecordEntry_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record",
              DisplayName = "Add Record of Entities")
    static void
    Add(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Record",
              DisplayName = "Has Record of Entities?")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Record",
              DisplayName = "Ensure Has Record of Entities")
    static bool
    Ensure(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static TArray<FCk_Handle>
    Get_AllRecordEntries(
        FCk_Handle InRecordHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static bool
    Get_HasRecordEntry(
        FCk_Handle InRecordHandle,
        FCk_Predicate_InHandle_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static FCk_Handle
    Get_RecordEntryIf(
        FCk_Handle InRecordHandle,
        FCk_Predicate_InHandle_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static void
    ForEachEntry(
        FCk_Handle InRecordHandle,
        FCk_Lambda_InHandle InFunc);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static void
    ForEachEntryIf(
        FCk_Handle InRecordHandle,
        FCk_Lambda_InHandle InFunc,
        FCk_Predicate_InHandle_OutResult InPredicate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static void
    Request_Connect(
        FCk_Handle InRecordHandle,
        FCk_Handle InRecordEntry);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static void
    Request_Disconnect(
        FCk_Handle InRecordHandle,
        FCk_Handle InRecordEntry);
};


// --------------------------------------------------------------------------------------------------------------------