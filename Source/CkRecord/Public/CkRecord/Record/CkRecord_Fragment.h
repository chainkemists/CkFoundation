#pragma once

#include "CkEcs/Concepts/CkConcepts.h"
#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkRecord/Record/CkRecord_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_RecordOfEntities_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <concepts::ValidHandleType T_HandleType>
    struct TFragment_RecordOfEntities
    {
    public:
        CK_GENERATED_BODY(TFragment_RecordOfEntities<T_HandleType>);

    public:
        friend class UCk_Utils_RecordOfEntities_UE;
        friend class FProcessor_RecordOfEntities_Destructor;
        friend class FProcessor_RecordEntry_Destructor;

        template <typename T_DerivedRecord>
        friend class TUtils_RecordOfEntities;

    public:
        // TODO: Use FCk_DebuggableEntity when available [OBS-845]
        using MaybeTypeSafeHandle = T_HandleType;
        using EntityType = MaybeTypeSafeHandle;
        using RecordEntriesType = TArray<EntityType>;

    private:
        // mutable because we lazily remove entries when performing a ForEach
        mutable RecordEntriesType _RecordEntries;
        ECk_Record_EntryHandlingPolicy _EntryHandlingPolicy = ECk_Record_EntryHandlingPolicy::Default;

    private:
        CK_PROPERTY(_RecordEntries);
        CK_PROPERTY(_EntryHandlingPolicy);

    public:
        CK_DEFINE_CONSTRUCTORS(TFragment_RecordOfEntities, _EntryHandlingPolicy);
    };
}

#define CK_DEFINE_RECORD_OF_ENTITIES(_NameOfRecord_, _HandleType_)     \
struct _NameOfRecord_ : public TFragment_RecordOfEntities<_HandleType_>\
{                                                                      \
    using TFragment_RecordOfEntities::TFragment_RecordOfEntities;      \
}

// --------------------------------------------------------------------------------------------------------------------
