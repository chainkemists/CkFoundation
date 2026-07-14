#include "CkEntityReplicationDriver_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h" // FTag_RepFragments_PendingApply (stall report)
#include "CkEcs/Persistence/CkPersistenceHydration.h"                            // FFragment_PendingHydration (stall report)
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_ReplicationDriver_FireOnDependentReplicationComplete);

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ReplicationDriver_FireOnDependentReplicationComplete::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle)
        -> void
    {
        // Fire-gating (spec §4.4, CTO note N2): do not fire until every replicated-fragment / hydration payload on
        // this entity AND its lifetime-dependents has drained — otherwise an OnReplicationComplete /
        // OnDependentsReplicationComplete consumer reads pre-apply state (esp. the §2.4 ConstructedThisFrame defer
        // window on initial subtree replication). Leave the fire tag (also MarkedDirtyBy) so the same-frame pump /
        // next tick retries. The dispatcher's 5s/2s timeout bounds entries a dispatcher on this net mode actually
        // drains — an entry NOTHING drains (e.g. PendingApply stamped on a world whose ClientOnly dispatcher never
        // runs) held the gate FOREVER and silently hung every OnReplicationComplete consumer (the post-v3-load HUD
        // hang, 2026-07-14). The stall report below makes that state loud and names the blocker.
        if (UCk_Utils_EntityReplicationDriver_UE::Get_HasUndrainedReplicatedFragments_IncludingDependents(InHandle))
        {
            auto& Stall = InHandle.AddOrGet<FFragment_RepDriver_FireGateStall>();
            Stall._Seconds += InDeltaT.Get_Seconds();

            constexpr auto StallReportThresholdSeconds = 10.0f; // past every dispatcher timeout (5s dev / 2s shipping)
            if (NOT Stall._Reported && Stall._Seconds > StallReportThresholdSeconds)
            {
                Stall._Reported = true;
                const auto Blocker =
                    UCk_Utils_EntityReplicationDriver_UE::TryGet_FirstUndrainedReplicatedFragmentsEntity_IncludingDependents(InHandle);
                const auto BlockedByPendingApply = ck::IsValid(Blocker) && Blocker.Has<FTag_RepFragments_PendingApply>();
                const auto BlockedByPendingHydration = ck::IsValid(Blocker) && Blocker.Has<FFragment_PendingHydration>();

                CK_TRIGGER_ENSURE(TEXT("OnReplicationComplete for [{}] has been gate-held for over [{}]s by [{}] "
                    "(PendingApply [{}], PendingHydration [{}]) — an undrained entry no dispatcher on this net mode "
                    "drains. Every consumer awaiting OnReplicationComplete on this entity (e.g. "
                    "Promise_OnActorEcsReady/ValuesReplicated) will never fire."),
                    InHandle, StallReportThresholdSeconds, Blocker, BlockedByPendingApply, BlockedByPendingHydration);
            }
            return;
        }

        InHandle.Try_Remove<FFragment_RepDriver_FireGateStall>();
        InHandle.Remove<MarkedDirtyBy>();

        if (NOT UUtils_Signal_OnReplicationComplete::HasFiredAtLeastOnce(InHandle))
        {
            ck::ecs::Verbose(TEXT("[EcsReadyPromise] Broadcasting OnReplicationComplete for [{}]"), InHandle);
            UUtils_Signal_OnReplicationComplete::Broadcast(InHandle, ck::MakePayload(InHandle));
        }

        UUtils_Signal_OnDependentsReplicationComplete::Broadcast(InHandle, ck::MakePayload(InHandle));
    }
}

// --------------------------------------------------------------------------------------------------------------------

