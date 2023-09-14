#pragma once

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_RecordOfEntities_UE;

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfEntities
    {
        friend class UCk_Utils_RecordOfEntities_UE;
        friend class FProcessor_RecordOfEntities_Destructor;
        friend class FProcessor_RecordEntry_Destructor;

        template <typename T_DerivedRecord>
        friend class TUtils_RecordOfEntities;

    public:
        CK_GENERATED_BODY(FFragment_RecordOfEntities);

        using EntityType = FCk_Entity;
        using RecordEntriesType = TSet<EntityType>;

    private:
        RecordEntriesType _RecordEntries;

    private:
        CK_PROPERTY(_RecordEntries);
    };
}

// --------------------------------------------------------------------------------------------------------------------
