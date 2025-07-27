#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include <Engine/DataTable.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkDataTable_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "UDataTable FDataTableRowHandle"))
class CKCORE_API UCk_Utils_DataTable_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DataTable_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|DataTable",
              DisplayName = "[Ck] Get Data Table Column Names")
    static TArray<FName>
    Get_ColumnNames(
        UDataTable* InDataTable);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|DataTable",
              DisplayName = "[Ck] Get Is Valid (DataTableRowHandle)",
              meta = (CompactNodeTitle = "IsValid"))
    static bool
    Get_IsValid_RowHandle(
        const FDataTableRowHandle& InDataTableRowHandle);
};

// --------------------------------------------------------------------------------------------------------------------
