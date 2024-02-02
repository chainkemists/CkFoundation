#include "CkRecord_Utils.h"

#include "CkRecord_Fragment.h"

#include "CkRecord/RecordEntry/CkRecordEntry_Fragment.h"
#include "CkRecord/RecordEntry/CkRecordEntry_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RecordOfEntities_UE::
    Add(
        FCk_Handle InHandle)
    -> void
{
    UtilsType::AddIfMissing(InHandle);
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return UtilsType::Has(InHandle);
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    return UtilsType::Ensure(InHandle);
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Get_HasValidEntry_If(
        FCk_Handle InRecordHandle,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> bool
{
    return UtilsType::Get_HasValidEntry_If(InRecordHandle, [&](FCk_Handle InHandle) -> bool
    {
        const FCk_SharedBool Result;
        InPredicate.Execute(InHandle, Result, InOptionalPayload);

        return *Result;
    });
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Get_ValidEntry_If(
        FCk_Handle InRecordHandle,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> FCk_Handle
{
    return UtilsType::Get_ValidEntry_If(InRecordHandle, [&](FCk_Handle InHandle) -> bool
    {
        const FCk_SharedBool Result;
        InPredicate.Execute(InHandle, Result, InOptionalPayload);

        return *Result;
    });
}

auto
    UCk_Utils_RecordOfEntities_UE::
    ForEach_ValidEntry(
        FCk_Handle& InAbilityOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InFunc)
    -> TArray<FCk_Handle>
{
    auto Entities = TArray<FCk_Handle>{};

    UtilsType::ForEach_ValidEntry(InAbilityOwnerEntity, [&](FCk_Handle InEntity)
    {
        if (InFunc.IsBound())
        { InFunc.Execute(InEntity, InOptionalPayload); }
        else
        { Entities.Emplace(InEntity); }
    });

    return Entities;
}

auto
    UCk_Utils_RecordOfEntities_UE::
    ForEach_ValidEntry_If(
        FCk_Handle& InRecordHandle,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InFunc,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> TArray<FCk_Handle>
{
    auto Entities = TArray<FCk_Handle>{};

    UtilsType::ForEach_ValidEntry_If(InRecordHandle,
    [&](FCk_Handle RecordEntryHandle)
    {
        if (InFunc.IsBound())
        { InFunc.Execute(RecordEntryHandle, InOptionalPayload); }
        else
        { Entities.Emplace(RecordEntryHandle); }
    },
    [&](FCk_Handle InRecordEntryHandle)
    {
        const FCk_SharedBool Result;
        InPredicate.Execute(InRecordEntryHandle, Result, InOptionalPayload);

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
