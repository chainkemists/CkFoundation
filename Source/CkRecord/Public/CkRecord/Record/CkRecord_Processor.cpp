#include "CkRecord_Processor.h"

#include "CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_RecordOfEntities_Destructor::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RecordOfEntities& InRecord)
        -> void
    {
        for (const auto RecordEntryEntity : InRecord.Get_RecordEntries())
        {
            auto RecordEntryHandle = MakeHandle(RecordEntryEntity, InHandle);

            auto& RecordEntryFragment = RecordEntryHandle.Get<FFragment_RecordEntry>();
            RecordEntryFragment._Records.Remove(InHandle.Get_Entity());
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
