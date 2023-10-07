#pragma once

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkRecord/Record/CkRecord_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_RecordOfEntities_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FFragment_RecordOfEntities
    {

    public:
        CK_GENERATED_BODY(FFragment_RecordOfEntities);

    public:
        friend class UCk_Utils_RecordOfEntities_UE;
        friend class FProcessor_RecordOfEntities_Destructor;
        friend class FProcessor_RecordEntry_Destructor;

        template <typename T_DerivedRecord>
        friend class TUtils_RecordOfEntities;

    public:
        // TODO: Use FCk_DebuggableEntity when available [OBS-845]
        using EntityType = FCk_Handle;
        using RecordEntriesType = TSet<EntityType>;

    private:
        RecordEntriesType _RecordEntries;
        ECk_Record_EntryHandlingPolicy _EntryHandlingPolicy = ECk_Record_EntryHandlingPolicy::Default;

    private:
        CK_PROPERTY(_RecordEntries);
        CK_PROPERTY(_EntryHandlingPolicy);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_RecordOfEntities, _EntryHandlingPolicy);
    };
}

// --------------------------------------------------------------------------------------------------------------------
