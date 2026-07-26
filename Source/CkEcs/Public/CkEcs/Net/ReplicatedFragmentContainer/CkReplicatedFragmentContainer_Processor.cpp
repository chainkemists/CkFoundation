#include "CkReplicatedFragmentContainer_Processor.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Persistence/CkPersistenceHydration_Processor.h" // ck::PendingApplyTimeoutSeconds
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_ReplicatedFragments_Dispatch);

namespace ck
{
    auto
        FProcessor_ReplicatedFragments_Dispatch::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>& InDriver) const
        -> void
    {
        // Defer entities composed this frame: their feature Setups drain in the pump AFTER FGroup_DeferredApply,
        // so applying now would be stomped. The pending tag stays; the pump (post-Setup) applies.
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

            const auto ApplyResult = Handler->NetApply(InHandle, Entry.Data, OldData);
            if (ApplyResult == ECk_Persistence_ApplyResult::Applied)
            {
                Entry._PendingApply = false;
                Entry._PendingForSeconds = 0.0f;
                Entry._LastAppliedData = Entry.Data;
                Entry._WasEverApplied = true;
                continue;
            }

            if (ApplyResult == ECk_Persistence_ApplyResult::Rejected)
            {
                Entry._PendingApply = false;
                Entry._PendingForSeconds = 0.0f;
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
