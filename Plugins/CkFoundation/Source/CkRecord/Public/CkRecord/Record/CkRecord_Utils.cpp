#include "CkRecord_Utils.h"

#include "CkRecord_Fragment.h"

#include "CkRecord/RecordEntry/CkRecordEntry_Fragment.h"
#include "CkRecord/RecordEntry/CkRecordEntry_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Utils_Record_UE::
    Add(FCk_Handle InHandle)
    -> void
{
    InHandle.Add<ck::FCk_Fragment_Record>();
}

auto
    UCk_Utils_Record_UE::
    Has(FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FCk_Fragment_Record>();
}

auto
    UCk_Utils_Record_UE::
    Ensure(FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have [{}]"),
        InHandle, ctti::nameof_v<ck::FCk_Fragment_Record>)
    { return false; }

    return true;
}

auto
    UCk_Utils_Record_UE::
    Get_AllRecordEntries(FCk_Handle InRecordHandle)
    -> TArray<FCk_Handle>
{
    const auto& Fragment = InRecordHandle.Get<ck::FCk_Fragment_Record>();

    auto RecordEntries = TArray<FCk_Handle>{};

    for(const auto& RecordEntry : Fragment.Get_RecordEntries())
    {
        RecordEntries.Emplace(ck::MakeHandle(RecordEntry, InRecordHandle));
    }
    return RecordEntries;
}

auto
    UCk_Utils_Record_UE::
    Get_HasRecordEntry(
        FCk_Handle InRecordHandle,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> bool
{
    const auto& Fragment = InRecordHandle.Get<ck::FCk_Fragment_Record>();

    for(const auto& RecordEntry : Fragment.Get_RecordEntries())
    {
        FCk_SharedBool OutResult;
        InPredicate.Execute(ck::MakeHandle(RecordEntry, InRecordHandle), OutResult);

        if (*OutResult)
        { return true; }
    }

    return false;
}

auto
    UCk_Utils_Record_UE::
    Get_RecordEntryIf(
        FCk_Handle InRecordHandle,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> FCk_Handle
{
    const auto& Fragment = InRecordHandle.Get<ck::FCk_Fragment_Record>();

    for(const auto& RecordEntry : Fragment.Get_RecordEntries())
    {
        const auto RecordEntryHandle = ck::MakeHandle(RecordEntry, InRecordHandle);

        FCk_SharedBool OutResult;
        InPredicate.Execute(RecordEntryHandle, OutResult);

        if (*OutResult)
        { return RecordEntryHandle; }
    }

    return {};
}

auto
    UCk_Utils_Record_UE::
    ForEachEntry(
        FCk_Handle InRecordHandle,
        FCk_Lambda_InHandle InFunc)
    -> void
{
    const auto& Fragment = InRecordHandle.Get<ck::FCk_Fragment_Record>();

    for(const auto& RecordEntry : Fragment.Get_RecordEntries())
    {
        const auto RecordEntryHandle = ck::MakeHandle(RecordEntry, InRecordHandle);
        InFunc.Execute(RecordEntryHandle);
    }
}

auto
    UCk_Utils_Record_UE::
    ForEachEntryIf(
        FCk_Handle InRecordHandle,
        FCk_Lambda_InHandle InFunc,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> void
{
    const auto& Fragment = InRecordHandle.Get<ck::FCk_Fragment_Record>();

    for(const auto& RecordEntry : Fragment.Get_RecordEntries())
    {
        const auto RecordEntryHandle = ck::MakeHandle(RecordEntry, InRecordHandle);

        FCk_SharedBool OutResult;
        InPredicate.Execute(RecordEntryHandle, OutResult);

        if (*OutResult)
        { InFunc.Execute(RecordEntryHandle); }
    }
}

auto
    UCk_Utils_Record_UE::
    Request_Connect(
        FCk_Handle InRecordHandle,
        FCk_Handle InRecordEntry)
    -> void
{
    if (NOT Ensure(InRecordHandle))
    { return; }

    auto& RecordFragment = InRecordHandle.Get<ck::FCk_Fragment_Record>();

    CK_ENSURE_IF_NOT(NOT RecordFragment.Get_RecordEntries().Contains(InRecordEntry.Get_Entity()),
        TEXT("The Record [{}] ALREADY contains the RecordEntry [{}]"), InRecordHandle, InRecordEntry)
    { return; }

    RecordFragment._RecordEntries.Emplace(InRecordEntry.Get_Entity());

    if (NOT UCk_Utils_RecordEntry_UE::Has(InRecordEntry))
    { UCk_Utils_RecordEntry_UE::Add(InRecordEntry); }

    auto& RecordEntryFragment = InRecordHandle.Get<ck::FCk_Fragment_RecordEntry>();
    RecordEntryFragment._Records.Emplace(InRecordHandle.Get_Entity());
}

auto
    UCk_Utils_Record_UE::
    Request_Disconnect(
        FCk_Handle InRecordHandle,
        FCk_Handle InRecordEntry)
    -> void
{
    if (NOT Ensure(InRecordHandle))
    { return; }

    if (NOT UCk_Utils_RecordEntry_UE::Ensure(InRecordEntry))
    { return; }

    {
        auto& RecordFragment = InRecordHandle.Get<ck::FCk_Fragment_Record>();
        const auto& RemovalSuccess = RecordFragment._RecordEntries.Remove(InRecordEntry.Get_Entity());

        CK_ENSURE_IF_NOT(RemovalSuccess,
            TEXT("The Record [{}] couldn't remove the RecordEntry [{}]. Does the RecordEntry exist in the Record?"),
            InRecordHandle,
            InRecordEntry)
        { return; }
    }

    {
        auto& RecordEntryFragment = InRecordHandle.Get<ck::FCk_Fragment_RecordEntry>();
        const auto& RemovalSuccess = RecordEntryFragment._Records.Remove(InRecordHandle.Get_Entity());

        CK_ENSURE_IF_NOT(RemovalSuccess,
            TEXT("The RecordEntry [{}] does NOT have the Record [{}] even though the Record had the RecordEntry. "
                "Somehow the RecordEntry was out of sync with the Record."),
            InRecordEntry,
            InRecordHandle)
        { return; }
    }
}
