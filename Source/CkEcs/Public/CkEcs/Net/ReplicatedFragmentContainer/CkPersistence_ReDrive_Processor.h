#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"

class UScriptStruct;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Per-entity progress marker for the restore re-drive. _Remaining holds the payload types still to seed on this
    // entity; it is drained over ticks and left EMPTY (not removed) once done — the emptied fragment IS the
    // done-marker, because FTag_Snapshot_JustRestored persists (multi-consumer) so completion cannot key on its
    // removal. NOT snapshotable — it is post-restore runtime bookkeeping and must never round-trip.
    struct FFragment_Persistence_ReDrivePending
    {
        CK_GENERATED_BODY(FFragment_Persistence_ReDrivePending);

    public:
        TArray<TWeakObjectPtr<const UScriptStruct>> _Remaining;
        float _PendingForSeconds = 0.0f;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Replaces the migrated features' *_ReplicateOnRestore family (Velocity/Acceleration/Attribute×5/TagSet/
    // MontagePlayer/AnimPlan): on every FTag_Snapshot_JustRestored entity, each registered handler with BOTH
    // Produce AND SeedContainer (the participation rule) re-seeds its replication container entry from live state —
    // reconstitution abstains Construct, so the entry + the entity-side TFragment_ContainerEntryRef the feature's
    // Replicate processor keys on are missing. Retries per-entity until each SeedContainer accepts (owner/driver
    // preconditions live inside SeedContainer); drops LOUDLY past the same 5s/2s window the net dispatcher uses.
    // Does NOT remove FTag_Snapshot_JustRestored (the 6 deferred restore processors + Phase-4A SM still key on it).
    class CKECS_API FProcessor_Persistence_ReDriveOnRestore : public ck_exp::TProcessor<
        FProcessor_Persistence_ReDriveOnRestore,
        FCk_Handle,
        FTag_Snapshot_JustRestored,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Replication;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        static constexpr auto LoadPolicy = ECk_ProcessorLoadPolicy::RunsDuringLoad; // load-gate kernel (spec §4.3)

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
