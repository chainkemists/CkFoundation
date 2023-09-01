#pragma once
#include "CkEcs/Entity/CkEntity.h"

#include "CkMacros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_RecordEntry_UE;
class UCk_Utils_RecordOfEntities_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /*
     * RecordEntry's ONLY purpose is to allow us to remove an entry from a Record of an Entity that is Pending Destroy.
*    * RecordEntry should NOT be used to link back back to the Record (and as such, a function like Get_Record does
*    * NOT exist)
     */
    struct FFragment_RecordEntry
    {
    public:
        CK_GENERATED_BODY(FFragment_RecordEntry);

        friend UCk_Utils_RecordEntry_UE;
        friend UCk_Utils_RecordOfEntities_UE;

        friend class FProcessor_RecordEntry_Destructor;
        friend class FProcessor_RecordOfEntities_Destructor;

        template <typename T_DerivedRecord>
        friend class TUtils_RecordOfEntities;

    public:
        using EntityType = FCk_Entity;
        using RecordsListType = TSet<EntityType>;

    private:
        RecordsListType _Records;

    private:
        CK_PROPERTY(_Records);
    };
}

// --------------------------------------------------------------------------------------------------------------------
