#include "CkRecordEntry_Utils.h"

#include "CkRecordEntry_Fragment.h"

#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RecordEntry_UE::
    Add(
        FCk_Handle& InHandle)
    -> void
{
    InHandle.Add<ck::FFragment_RecordEntry>();
}

auto
    UCk_Utils_RecordEntry_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_RecordEntry>();
}

auto
    UCk_Utils_RecordEntry_UE::
    Ensure(
        const FCk_Handle& InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have a [{}]"),
        InHandle, ck::TypeToString<ck::FFragment_RecordEntry>)
    { return false; }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

