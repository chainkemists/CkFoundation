#pragma once

#include "CkCore/Enums/CkEnums.h"          // ECk_MinMaxCurrent, ECk_AddedOrNot
#include "CkEcs/Handle/CkHandle.h"          // FCk_Handle, ck::Is_NOT_Valid
#include "CkEcs/Net/CkNet_Utils.h"          // TryAddContainerFragment, Get_LifetimeOwner; transitively ECk_Persistence_ApplyResult
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies

#include "CkAttribute/CkAttribute_Log.h"    // ck::attribute::Verbose (component-drift skip)
#include "CkLabel/CkLabel_Utils.h"          // UCk_Utils_GameplayLabel_UE::Get_Label (value-emitting Produce)

#include <InstancedStruct.h>
#include <Misc/Optional.h>

// --------------------------------------------------------------------------------------------------------------------
// Shared Produce / HydrationApply for the attribute family (Float/Byte/Integer/Vector/Rotator). SAVE is keyed
// PER-ATTRIBUTE-ENTITY while the net Apply stays OWNER-keyed — that asymmetry is documented in CkAttribute/CLAUDE.md.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::attribute_restore
{
    template <template <ECk_MinMaxCurrent> class T_DerivedAttribute, typename T_RepDataStruct>
    auto
        Produce(
            FCk_Handle& InEntity) -> TOptional<FInstancedStruct>
    {
        using Current = T_DerivedAttribute<ECk_MinMaxCurrent::Current>;
        using Min     = T_DerivedAttribute<ECk_MinMaxCurrent::Min>;
        using Max     = T_DerivedAttribute<ECk_MinMaxCurrent::Max>;

        if (NOT InEntity.Has<Current>())
        { return {}; }

        auto Data = T_RepDataStruct{};
        using EntryType = typename decltype(Data.Attributes)::ElementType;

        const auto& AttributeName = UCk_Utils_GameplayLabel_UE::Get_Label(InEntity);

        const auto& Cur = InEntity.Get<Current>();
        Data.Attributes.Emplace(EntryType{AttributeName, Cur.Get_Base(), Cur.Get_Final(), Current::ComponentTagType});

        if (InEntity.Has<Min>())
        {
            const auto& MinFrag = InEntity.Get<Min>();
            Data.Attributes.Emplace(EntryType{AttributeName, MinFrag.Get_Base(), MinFrag.Get_Final(), Min::ComponentTagType});
        }
        if (InEntity.Has<Max>())
        {
            const auto& MaxFrag = InEntity.Get<Max>();
            Data.Attributes.Emplace(EntryType{AttributeName, MaxFrag.Get_Base(), MaxFrag.Get_Final(), Max::ComponentTagType});
        }

        return FInstancedStruct::Make(MoveTemp(Data));
    }

    // ALL NotReady exits must precede any mutation: ApplyReplicated*Entry's Add_Revocable creates a NEW modifier per
    // call, so a mutate-then-NotReady retry would stack a second replication modifier.
    template <template <ECk_MinMaxCurrent> class T_DerivedAttribute, typename T_RepDataStruct, typename T_UtilsType, typename T_ApplyEntryFn>
    auto
        HydrationApply(
            FCk_Handle& InEntity,
            const FInstancedStruct& InNew,
            T_ApplyEntryFn InApplyEntry) -> ECk_Persistence_ApplyResult
    {
        using Current = T_DerivedAttribute<ECk_MinMaxCurrent::Current>;
        using Min     = T_DerivedAttribute<ECk_MinMaxCurrent::Min>;
        using Max     = T_DerivedAttribute<ECk_MinMaxCurrent::Max>;

        if (NOT InEntity.Has<Current>())
        { return ECk_Persistence_ApplyResult::NotReady; }

        auto AttributeEntity = T_UtilsType::Cast(InEntity);

        for (const auto& Entry : InNew.Get<T_RepDataStruct>().Attributes)
        {
            const auto Component = Entry.Get_Component();
            const auto ComponentComposed =
                   Component == ECk_MinMaxCurrent::Current
                || (Component == ECk_MinMaxCurrent::Min && InEntity.Has<Min>())
                || (Component == ECk_MinMaxCurrent::Max && InEntity.Has<Max>());

            if (NOT ComponentComposed)
            {
                ck::attribute::Verbose(
                    TEXT("Snapshot hydration: attribute [{}] carries a saved component absent post-Construct — ")
                    TEXT("skipping that entry (content drifted since the save)."),
                    InEntity);
                continue;
            }

            InApplyEntry(AttributeEntity, Entry);
        }

        return ECk_Persistence_ApplyResult::Applied;
    }
}
