#include "CkRecord_Processor.h"

#include "CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FCk_Processor_Record_Destructor::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Record& InRecord)
        -> void
    {
        for (auto RecordEntryEntity : InRecord.Get_RecordEntries())
        {
            const auto RecordEntryHandle = MakeHandle(RecordEntryEntity, InHandle);
            UCk_Utils_Record_UE::Request_Disconnect(InHandle, RecordEntryHandle);
        }
    }
}
