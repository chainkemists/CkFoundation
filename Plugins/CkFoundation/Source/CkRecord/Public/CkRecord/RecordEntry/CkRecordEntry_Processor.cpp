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
            const FCk_Fragment_RecordEntry& InRecordEntry)
        -> void
    {
        for (const auto RecordEntity : InRecordEntry.Get_Records())
        {
            const auto RecordHandle = MakeHandle(RecordEntity, InHandle);
            UCk_Utils_Record_UE::Request_Disconnect(RecordHandle, InHandle);
        }
    }
}
