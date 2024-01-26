#include "CkDataTable_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DataTable_UE::
    Get_ColumnNames(
        UDataTable* InDataTable)
    -> TArray<FName>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDataTable),
        TEXT("Invalid DataTable when trying to retrieve its Column Names"))
    { return {}; }

    const auto& RowStruct = InDataTable->GetRowStruct();

    CK_ENSURE_IF_NOT(ck::IsValid(InDataTable),
        TEXT("Cannot retrieve the Column Names of DataTable [{}] because it has an INVALID RowStruct"),
        InDataTable)
    { return {}; }

    TArray<FName> ColumnNames;

    for (auto It = TFieldIterator<FProperty>(RowStruct); It; ++It)
    {
        const auto* Property = *It;

        if (ck::Is_NOT_Valid(Property, ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        const auto& ColumnName = Property->GetFName();
        ColumnNames.Add(ColumnName);
    }

    return ColumnNames;
}

auto
    UCk_Utils_DataTable_UE::
    Get_IsValid_RowHandle(
        const FDataTableRowHandle& InDataTableRowHandle)
    -> bool
{
    return ck::IsValid(InDataTableRowHandle);
}

// --------------------------------------------------------------------------------------------------------------------
