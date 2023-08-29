#pragma once

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Record_UE;

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct FCk_Fragment_Record
    {
        friend class UCk_Utils_Record_UE;
        friend class FCk_Processor_Record_Destructor;
        friend class FCk_Processor_RecordEntry_Destructor;

    public:
        CK_GENERATED_BODY(FCk_Fragment_Record);

        using EntityType = FCk_Entity;
        using RecordEntriesType = TSet<EntityType>;

    private:
        RecordEntriesType _RecordEntries;

    private:
        CK_PROPERTY(_RecordEntries);
    };
}

// --------------------------------------------------------------------------------------------------------------------
