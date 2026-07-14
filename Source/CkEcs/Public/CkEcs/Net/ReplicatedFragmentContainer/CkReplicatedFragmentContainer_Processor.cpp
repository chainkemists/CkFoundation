#include "CkReplicatedFragmentContainer_Processor.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_ReplicatedFragments_Dispatch);

namespace ck
{
    // Past this, an entry whose Apply keeps returning NotReady is dropped LOUDLY — the feature it
    // targets was never composed on this client. Mirrors the EntityScript pending-replication
    // timeout pattern (CkEntityScript_Processor.cpp).
#if UE_BUILD_SHIPPING
    constexpr auto PendingApplyTimeoutSeconds = 2.0f;
#else
    constexpr auto PendingApplyTimeoutSeconds = 5.0f;
#endif

    auto
        FProcessor_ReplicatedFragments_Dispatch::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>& InDriver) const
        -> void
    {
        // Defer entities composed this frame (Phase 2 §2.4): their feature Setups drain in the pump AFTER
        // FGroup_DeferredApply, so applying now would be stomped. The pending tag stays; the pump (post-Setup) applies.
        if (InHandle.Has<FTag_EntityScript_ConstructedThisFrame>())
        { return; }

        if (ck::Is_NOT_Valid(InDriver))
        {
            InHandle.Remove<FTag_RepFragments_PendingApply>();
            return;
        }

        auto& Fragments = InDriver->_Fragments;

        for (const auto& Removed : Fragments._PendingRemovals)
        {
            const auto* Handler = FCk_PersistenceHandlerRegistry::Resolve(Removed.GetScriptStruct());
            if (Handler == nullptr || NOT Handler->NetRemove)
            { continue; }

            Handler->NetRemove(InHandle);
        }
        Fragments._PendingRemovals.Reset();

        auto AnyStillPending = false;

        for (auto& Entry : Fragments._Items)
        {
            if (NOT Entry._PendingApply)
            { continue; }

            const auto* Handler = FCk_PersistenceHandlerRegistry::Resolve(Entry.Data.GetScriptStruct());
            if (Handler == nullptr || NOT Handler->NetApply)
            {
                // Defensive: an entry can only become pending if its handler had Apply at marking
                // time, so this indicates registry churn. Drop rather than retry forever.
                Entry._PendingApply = false;
                continue;
            }

            const auto OldData = Entry._WasEverApplied
                ? TOptional<FInstancedStruct>{Entry._LastAppliedData}
                : TOptional<FInstancedStruct>{};

            if (Handler->NetApply(InHandle, Entry.Data, OldData) == ECk_Persistence_ApplyResult::Applied)
            {
                Entry._PendingApply = false;
                Entry._PendingForSeconds = 0.0f;
                Entry._LastAppliedData = Entry.Data;
                Entry._WasEverApplied = true;
                continue;
            }

            Entry._PendingForSeconds += InDeltaT.Get_Seconds();

            if (Entry._PendingForSeconds >= PendingApplyTimeoutSeconds)
            {
                const auto TypeName = ck::IsValid(Entry.Data.GetScriptStruct())
                    ? Entry.Data.GetScriptStruct()->GetName()
                    : FString{TEXT("<invalid type>")};

                CK_TRIGGER_ENSURE(
                    TEXT("Replicated fragment [{}] on entity [{}] was never applied: Apply kept "
                         "returning NotReady for [{}]s — the feature it targets was never composed "
                         "on this client. Dropping the entry."),
                    TypeName, InHandle, Entry._PendingForSeconds);

                Entry._PendingApply = false;
                Entry._PendingForSeconds = 0.0f;
                continue;
            }

            AnyStillPending = true;
        }

        if (NOT AnyStillPending)
        { InHandle.Remove<FTag_RepFragments_PendingApply>(); }
    }
}

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Hydration_Dispatch);

namespace ck::persistence_apply
{
    auto
        ApplyOne(
            FCk_Handle& InEntity,
            const FInstancedStruct& InData,
            const TOptional<FInstancedStruct>& InOldData,
            float& InOutPendingForSeconds,
            FCk_Time InDeltaT)
        -> EApplyOutcome
    {
        const auto* Handler = FCk_PersistenceHandlerRegistry::Resolve(InData.GetScriptStruct());
        if (Handler == nullptr || NOT Handler->HydrationApply)
        {
            // No HydrationApply — either registry churn or a handler that never declared its load path. Drop
            // rather than retry forever.
            return EApplyOutcome::DroppedTimeout;
        }

        if (Handler->HydrationApply(InEntity, InData, InOldData) == ECk_Persistence_ApplyResult::Applied)
        { return EApplyOutcome::Applied; }

        InOutPendingForSeconds += InDeltaT.Get_Seconds();

        if (InOutPendingForSeconds >= ck::PendingApplyTimeoutSeconds)
        {
            const auto TypeName = ck::IsValid(InData.GetScriptStruct())
                ? InData.GetScriptStruct()->GetName()
                : FString{TEXT("<invalid type>")};

            CK_TRIGGER_ENSURE(
                TEXT("Hydration payload [{}] on entity [{}] was never applied: Apply kept returning NotReady for "
                     "[{}]s — the feature it targets was never composed. Dropping the entry."),
                TypeName, InEntity, InOutPendingForSeconds);

            return EApplyOutcome::DroppedTimeout;
        }

        return EApplyOutcome::StillPending;
    }
}

namespace ck
{
    auto
        FProcessor_Hydration_Dispatch::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        // [P2-D1] This dispatcher runs LAST in FGroup_DeferredApply (RunAfter the net dispatcher). Iterate + apply, THEN
        // clear FTag_EntityScript_ConstructedThisFrame registry-wide — by now BOTH dispatchers have skipped the
        // constructed-this-frame entities in this main pass, so clearing lets the pump (post-Setup) / next frame
        // re-dispatch them without a stomp. Idiom: CkIsmRenderer_Processor.cpp.
        TProcessor::DoTick(InDeltaT);
        _TransientEntity.Clear<FTag_EntityScript_ConstructedThisFrame>();
    }

    auto
        FProcessor_Hydration_Dispatch::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_PendingHydration& InPending) const
        -> void
    {
        // Defer entities composed this frame (Phase 2 §2.4) — same rationale as the net dispatcher.
        if (InHandle.Has<FTag_EntityScript_ConstructedThisFrame>())
        { return; }

        auto AnyStillPending = false;

        // Iterate back-to-front so applied/dropped entries can be removed in place.
        for (auto Index = InPending._Entries.Num() - 1; Index >= 0; --Index)
        {
            // Hydration has no per-entry coalescing (unlike the net FastArray) — the Old side is always unset.
            // ApplyOne dispatches through the handler's HydrationApply slot (the load-path applier); the choice of
            // slot is now structural (which callback), not a runtime scope query.
            const auto Outcome = ck::persistence_apply::ApplyOne(
                InHandle, InPending._Entries[Index], TOptional<FInstancedStruct>{}, InPending._PendingForSeconds, InDeltaT);

            if (Outcome == ck::persistence_apply::EApplyOutcome::StillPending)
            { AnyStillPending = true; }
            else
            { InPending._Entries.RemoveAt(Index); } // Applied or dropped-past-timeout
        }

        if (NOT AnyStillPending)
        { InPending._PendingForSeconds = 0.0f; }

        if (InPending._Entries.IsEmpty())
        {
            InHandle.Remove<FTag_Hydration_PendingApply>();
            InHandle.Remove<FFragment_PendingHydration>();
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
