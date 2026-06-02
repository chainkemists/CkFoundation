#pragma once

#include "CkEcs/Concepts/CkConcepts.h"
#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkRecord/Record/CkRecord_Fragment_Data.h"

// SerializeSnapshot routes each record-entry handle through FSnapshotContext::Snapshot_Handle, so the
// complete FSnapshotContext type is required here (template body lives in the header).
#include "CkSnapshot/Context/CkSnapshot_Context.h"

class FArchive;

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
        using IsSnapshotable = void;

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
        using RecordEntriesTagNamePairType = TPair<FName, EntityType>;
        using RecordEntriesTagNamePairsArrayType = TArray<RecordEntriesTagNamePairType>;

    private:
        // mutable because we lazily remove entries when performing a ForEach
        mutable RecordEntriesType _RecordEntries;
        mutable RecordEntriesTagNamePairsArrayType _RecordEntriesTagNamePairs;
        ECk_Record_EntryHandlingPolicy _EntryHandlingPolicy = ECk_Record_EntryHandlingPolicy::Default;

    private:
        CK_PROPERTY(_RecordEntries);
        CK_PROPERTY(_RecordEntriesTagNamePairs);
        CK_PROPERTY(_EntryHandlingPolicy);

    public:
        CK_DEFINE_CONSTRUCTORS(TFragment_RecordOfEntities, _EntryHandlingPolicy);

    public:
        // Tier-C: every record entry is an entity handle and MUST be remapped through FSnapshotContext.
        // _RecordEntriesTagNamePairs is the persistent label->entry index (maintained imperatively as labeled
        // entries are added/removed, NOT lazily rebuilt), so it is round-tripped too — both the FName label and
        // the handle remap. _EntryHandlingPolicy is a plain enum.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx) -> void
        {
            auto NumEntries = _RecordEntries.Num();
            InAr << NumEntries;
            if (InAr.IsLoading())
            { _RecordEntries.SetNum(NumEntries); }

            for (auto& Entry : _RecordEntries)
            { InCtx.Snapshot_Handle(InAr, Entry); }

            auto NumPairs = _RecordEntriesTagNamePairs.Num();
            InAr << NumPairs;
            if (InAr.IsLoading())
            { _RecordEntriesTagNamePairs.SetNum(NumPairs); }

            for (auto& Pair : _RecordEntriesTagNamePairs)
            {
                InAr << Pair.Key;
                InCtx.Snapshot_Handle(InAr, Pair.Value);
            }

            auto PolicyRaw = static_cast<uint8>(_EntryHandlingPolicy);
            InAr << PolicyRaw;
            _EntryHandlingPolicy = static_cast<ECk_Record_EntryHandlingPolicy>(PolicyRaw);
        }
    };
}

#define CK_DEFINE_RECORD_OF_ENTITIES(_NameOfRecord_, _HandleType_)         \
struct _NameOfRecord_ : public ck::TFragment_RecordOfEntities<_HandleType_>\
{                                                                          \
    using TFragment_RecordOfEntities::TFragment_RecordOfEntities;          \
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    // NOTE: this _should_ be in CkEntityExtensions but then we have a circular dependency
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfEntityExtensions, FCk_Handle_EntityExtension);

    // --------------------------------------------------------------------------------------------------------------------
}
