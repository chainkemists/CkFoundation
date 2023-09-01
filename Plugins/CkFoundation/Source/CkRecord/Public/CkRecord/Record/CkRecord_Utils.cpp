#include "CkRecord_Utils.h"

#include "CkRecord_Fragment.h"

#include "CkRecord/RecordEntry/CkRecordEntry_Fragment.h"
#include "CkRecord/RecordEntry/CkRecordEntry_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_Utils_RecordOfEntities_UE::InternalUtils_Type UCk_Utils_RecordOfEntities_UE::_Utils;

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RecordOfEntities_UE::
    Add(FCk_Handle InHandle)
    -> void
{
    _Utils.Add(InHandle);
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Has(FCk_Handle InHandle)
    -> bool
{
    return _Utils.Has(InHandle);
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Ensure(FCk_Handle InHandle)
    -> bool
{
    return _Utils.Ensure(InHandle);
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Get_AllRecordEntries(FCk_Handle InRecordHandle)
    -> TArray<FCk_Handle>
{
    return _Utils.Get_AllRecordEntries(InRecordHandle);
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Get_HasRecordEntry(
        FCk_Handle InRecordHandle,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> bool
{
    return _Utils.Get_HasRecordEntry(InRecordHandle, [&](FCk_Handle InHandle) -> bool
    {
        FCk_SharedBool OutResult;
        InPredicate.Execute(InHandle, OutResult);

        return *OutResult;
    });
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Get_RecordEntryIf(
        FCk_Handle InRecordHandle,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> FCk_Handle
{
    return _Utils.Get_RecordEntryIf(InRecordHandle, [&](FCk_Handle InHandle) -> bool
    {
        FCk_SharedBool OutResult;
        InPredicate.Execute(InHandle, OutResult);

        return *OutResult;
    });
}

auto
    UCk_Utils_RecordOfEntities_UE::
    ForEachEntry(
        FCk_Handle InRecordHandle,
        FCk_Lambda_InHandle InFunc)
    -> void
{
    return _Utils.ForEachEntry(InRecordHandle, [&](FCk_Handle InHandle)
    {
        InFunc.Execute(InHandle);
    });
}

auto
    UCk_Utils_RecordOfEntities_UE::
    ForEachEntryIf(
        FCk_Handle InRecordHandle,
        FCk_Lambda_InHandle InFunc,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> void
{
    return _Utils.ForEachEntryIf(InRecordHandle,
    [&](FCk_Handle RecordEntryHandle)
    {
        InFunc.Execute(RecordEntryHandle);
    },
    [&](FCk_Handle InRecordEntryHandle)
    {
        const FCk_SharedBool OutResult;
        InPredicate.Execute(InRecordEntryHandle, OutResult);

        return *OutResult;
    });
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Request_Connect(
        FCk_Handle InRecordHandle,
        FCk_Handle InRecordEntry)
    -> void
{
    _Utils.Request_Connect(InRecordHandle, InRecordEntry);
}

auto
    UCk_Utils_RecordOfEntities_UE::
    Request_Disconnect(
        FCk_Handle InRecordHandle,
        FCk_Handle InRecordEntry)
    -> void
{
    _Utils.Request_Disconnect(InRecordHandle, InRecordEntry);
}
