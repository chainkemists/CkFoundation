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
        for (const auto RecordEntryEntity : InRecord.Get_RecordEntries())
        {
            auto RecordEntryHandle = MakeHandle(RecordEntryEntity, InHandle);

            auto& RecordEntryFragment = RecordEntryHandle.Get<FCk_Fragment_RecordEntry>();
            RecordEntryFragment._Records.Remove(InHandle.Get_Entity());
        }
    }
}
