#include "CkRecordEntry_Processor.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------


namespace ck
{
    auto
        FCk_Processor_RecordEntry_Destructor::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const FFragment_RecordEntry& InRecordEntry)
        -> void
    {
        for (const auto RecordEntity : InRecordEntry.Get_Records())
        {
            auto RecordHandle = MakeHandle(RecordEntity, InHandle);

            auto& RecordFragment = RecordHandle.Get<FFragment_Record>();
            RecordFragment._RecordEntries.Remove(InHandle.Get_Entity());
        }
    }
}
