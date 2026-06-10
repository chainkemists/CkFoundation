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
        if (ck::Is_NOT_Valid(InDriver))
        {
            InHandle.Remove<FTag_RepFragments_PendingApply>();
            return;
        }

        auto& Fragments = InDriver->_Fragments;

        for (const auto& Removed : Fragments._PendingRemovals)
        {
            const auto* Handler = FCk_ReplicatedFragmentHandlerRegistry::Resolve(Removed.GetScriptStruct());
            if (Handler == nullptr || NOT Handler->Remove)
            { continue; }

            Handler->Remove(InHandle);
        }
        Fragments._PendingRemovals.Reset();

        auto AnyStillPending = false;

        for (auto& Entry : Fragments._Items)
        {
            if (NOT Entry._PendingApply)
            { continue; }

            const auto* Handler = FCk_ReplicatedFragmentHandlerRegistry::Resolve(Entry.Data.GetScriptStruct());
            if (Handler == nullptr || NOT Handler->Apply)
            {
                // Defensive: an entry can only become pending if its handler had Apply at marking
                // time, so this indicates registry churn. Drop rather than retry forever.
                Entry._PendingApply = false;
                continue;
            }

            const auto OldData = Entry._WasEverApplied
                ? TOptional<FInstancedStruct>{Entry._LastAppliedData}
                : TOptional<FInstancedStruct>{};

            if (Handler->Apply(InHandle, Entry.Data, OldData) == ECk_RepFragment_ApplyResult::Applied)
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
