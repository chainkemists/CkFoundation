#include "CkRecordEntry_Processor.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_RecordEntry_Destructor::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const FFragment_RecordEntry& InRecordEntry) const
        -> void
    {
        for (const auto RecordEntity : InRecordEntry.Get_Records())
        {
            auto RecordHandle = MakeHandle(RecordEntity, InHandle);

            auto& RecordFragment = RecordHandle.Get<FFragment_RecordOfEntities>();
            RecordFragment._RecordEntries.Remove(InHandle.Get_Entity());
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
