#include "CkRecordProcessorInjector.h"

#include "CkRecord/RecordEntry/CkRecordEntry_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Record_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_RecordEntry_Destructor>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
