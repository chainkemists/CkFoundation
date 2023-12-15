#include "CkRecord_Utils.h"

#include "CkRecord_Fragment.h"

#include "CkRecord/RecordEntry/CkRecordEntry_Fragment.h"
#include "CkRecord/RecordEntry/CkRecordEntry_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RecordOfEntities_UE::
    Add(FCk_Handle InHandle)
    -> void
{
    UtilsType::AddIfMissing(InHandle);
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Has(FCk_Handle InHandle)
    -> bool
{
    return UtilsType::Has(InHandle);
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Ensure(FCk_Handle InHandle)
    -> bool
{
    return UtilsType::Ensure(InHandle);
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Get_HasRecordEntry(
        FCk_Handle InRecordHandle,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> bool
{
    return UtilsType::Get_HasRecordEntry(InRecordHandle, [&](FCk_Handle InHandle) -> bool
    {
        const FCk_SharedBool Result;
        InPredicate.Execute(InHandle, Result);

        return *Result;
    });
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Get_RecordEntryIf(
        FCk_Handle InRecordHandle,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> FCk_Handle
{
    return UtilsType::Get_RecordEntryIf(InRecordHandle, [&](FCk_Handle InHandle) -> bool
    {
        const FCk_SharedBool Result;
        InPredicate.Execute(InHandle, Result);

        return *Result;
    });
}

auto
    UCk_Utils_RecordOfEntities_UE::
    ForEach_ValidEntry(
        FCk_Handle                 InAbilityOwnerEntity,
        const FCk_Lambda_InHandle& InFunc)
    -> TArray<FCk_Handle>
{
    auto Entities = TArray<FCk_Handle>{};

    UtilsType::ForEach_ValidEntry(InAbilityOwnerEntity, [&](FCk_Handle InEntity)
    {
        if (InFunc.IsBound())
        { InFunc.Execute(InEntity); }
        else
        { Entities.Emplace(InEntity); }
    });

    return Entities;
}

auto
    UCk_Utils_RecordOfEntities_UE::
    ForEach_ValidEntry_If(
        FCk_Handle InRecordHandle,
        const FCk_Lambda_InHandle& InFunc,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> TArray<FCk_Handle>
{
    auto Entities = TArray<FCk_Handle>{};

    UtilsType::ForEach_ValidEntry_If(InRecordHandle,
    [&](FCk_Handle RecordEntryHandle)
    {
        if (InFunc.IsBound())
        { InFunc.Execute(RecordEntryHandle); }
        else
        { Entities.Emplace(RecordEntryHandle); }
    },
    [&](FCk_Handle InRecordEntryHandle)
    {
        const FCk_SharedBool Result;
        InPredicate.Execute(InRecordEntryHandle, Result);

        return *Result;
    });

    return Entities;
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Request_Connect(
        FCk_Handle InRecordHandle,
        FCk_Handle InRecordEntry)
    -> void
{
    UtilsType::Request_Connect(InRecordHandle, InRecordEntry);
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Request_Disconnect(
        FCk_Handle InRecordHandle,
        FCk_Handle InRecordEntry)
    -> void
{
    UtilsType::Request_Disconnect(InRecordHandle, InRecordEntry);
}

// --------------------------------------------------------------------------------------------------------------------
