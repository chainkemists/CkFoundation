#pragma once
#include "CkEcs/Entity/CkEntity.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

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

    public:
        friend UCk_Utils_RecordEntry_UE;
        friend UCk_Utils_RecordOfEntities_UE;

        friend class FProcessor_RecordEntry_Destructor;
        friend class FProcessor_RecordOfEntities_Destructor;

        template <typename T_DerivedRecord>
        friend class TUtils_RecordOfEntities;

    public:
        // TODO: Use FCk_DebuggableEntity when available [OBS-845]
        using HandleType = FCk_Handle;
        using EntityType = HandleType;
        using RecordsListType = TSet<EntityType>;

        using DestructionCleanupFuncType = TFunction<void(FCk_Handle, FCk_Handle)>;
        using DestructionCleanupFuncMap = TMap<EntityType, DestructionCleanupFuncType>;
        using DestructionCleanupFuncPair = TPair<EntityType, DestructionCleanupFuncType>;

    private:
        RecordsListType _Records;
        DestructionCleanupFuncMap _DisconnectionFuncs;

    private:
        CK_PROPERTY(_Records);
    };
}

// --------------------------------------------------------------------------------------------------------------------
