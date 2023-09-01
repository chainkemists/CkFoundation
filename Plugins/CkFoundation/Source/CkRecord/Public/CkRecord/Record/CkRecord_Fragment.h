#pragma once

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Record_UE;

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_Record
    {
        friend class UCk_Utils_Record_UE;
        friend class FCk_Processor_Record_Destructor;
        friend class FCk_Processor_RecordEntry_Destructor;

        template <typename T_DerivedRecord>
        friend class TCk_Utils_Record;

    public:
        CK_GENERATED_BODY(FFragment_Record);

        using EntityType = FCk_Entity;
        using RecordEntriesType = TSet<EntityType>;

    private:
        RecordEntriesType _RecordEntries;

    private:
        CK_PROPERTY(_RecordEntries);
    };
}

// --------------------------------------------------------------------------------------------------------------------
