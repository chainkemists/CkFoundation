#pragma once

#include "CkCore/Enums/CkEnums.h"          // ECk_MinMaxCurrent, ECk_AddedOrNot
#include "CkEcs/Handle/CkHandle.h"          // FCk_Handle, ck::Is_NOT_Valid
#include "CkEcs/Handle/CkHandle_TypeSafe.h" // ck::StaticCast (hydration apply)
#include "CkEcs/Net/CkNet_Utils.h"          // TryAddContainerFragment, Get_LifetimeOwner; transitively FCk_HydrationApplyScope + ECk_RepFragment_ApplyResult
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h" // RegisterLazyTyped<T> body

#include "CkAttribute/CkAttribute_Log.h"    // ck::attribute::Verbose (component-drift skip)
#include "CkLabel/CkLabel_Utils.h"          // UCk_Utils_GameplayLabel_UE::Get_Label (value-emitting Produce)

#include <InstancedStruct.h>
#include <Misc/Optional.h>

// --------------------------------------------------------------------------------------------------------------------
// Shared Produce / hydration-apply for the attribute family (Float/Byte/Integer/Vector/Rotator), used by each kind's
// registrar.
//
// v3 SAVE (Produce) is VALUE-EMITTING and keyed PER-ATTRIBUTE-ENTITY (Produce fires on the entity holding the Current
// component). It emits this attribute's own Base/Final for each composed component (Current always; Min/Max if present)
// byte-identically to the wire-builder TProcessor_Attribute_Replicate (CkAttribute_Processor.inl.h). On load
// FProcessor_Hydration_Dispatch calls Apply(ThisAttributeEntity, payload) under FCk_HydrationApplyScope; the shared
// TryHydrationApply below writes the value AUTHORITY-side via the kind's ApplyReplicated*Entry (the same path the
// client net drain uses). The resulting deferred Request_* re-arm FTag_MayRequireReplication, so
// FProcessor_Attribute_Replicate re-publishes to post-load clients — no explicit owner-container refill needed (the
// owner container already exists on the freshly-Constructed owner). The net Apply below the branch is OWNER-keyed and
// untouched (byte-identical wire).
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

    // Save-load hydration (Phase 4B) — shared authority-side branch for every attribute kind. Returns UNSET when NOT
    // under hydration scope: the caller falls through to the unchanged OWNER-keyed net Apply (byte-identical wire).
    // Under hydration, InEntity IS the attribute entity (per-entity Produce keying), so write each saved component's
    // value directly to it via the kind's ApplyReplicated*Entry (the same path the client net drain uses).
    // ALL NotReady exits precede any mutation: ApplyReplicated*Entry's Add_Revocable creates a NEW modifier per call,
    // so a mutate-then-NotReady retry would stack a second replication modifier. The only NotReady is the Has<Current>
    // guard before any write; a saved component the re-Constructed attribute no longer composes is warn+skip (its
    // Get_/Request_ would ensure on the missing component, and composition is synchronous so absence is final).
    template <template <ECk_MinMaxCurrent> class T_DerivedAttribute, typename T_RepDataStruct, typename T_ApplyEntryFn>
    auto
        TryHydrationApply(
            FCk_Handle& InEntity,
            const FInstancedStruct& InNew,
            T_ApplyEntryFn InApplyEntry) -> TOptional<ECk_RepFragment_ApplyResult>
    {
        if (NOT FCk_HydrationApplyScope::Get_IsActive())
        { return {}; }

        using Current = T_DerivedAttribute<ECk_MinMaxCurrent::Current>;
        using Min     = T_DerivedAttribute<ECk_MinMaxCurrent::Min>;
        using Max     = T_DerivedAttribute<ECk_MinMaxCurrent::Max>;

        if (NOT InEntity.Has<Current>())
        { return ECk_RepFragment_ApplyResult::NotReady; }

        auto AttributeEntity = ck::StaticCast<typename Current::HandleType>(InEntity);

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

        return ECk_RepFragment_ApplyResult::Applied;
    }
}
