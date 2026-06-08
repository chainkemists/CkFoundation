#pragma once

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/OwningActor/CkActorSpawnIntent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Respawns + re-bridges bridged actors for entities a snapshot restore brought back. The restore stamps
    // FTag_ActorRespawn_Pending on each such entity (DoStamp_RespawnMarkers); this processor consumes it.
    //
    // It iterates the DATA fragment FFragment_ActorSpawnIntent (default deletion policy → clean, tombstone-free
    // iteration) and POINT-QUERIES the marker via Has<FTag_ActorRespawn_Pending>(). It deliberately does NOT
    // iterate a view over the marker tag itself: that tag's storage uses entt's in_place deletion policy (so the
    // tag can be Clear()'d freely), and a single-component view over an in_place storage surfaces TOMBSTONE slots
    // instead of the live entry. Point queries (Has/contains) on in_place storage are correct; only iteration is not.
    //
    // ForEachEntity COLLECTS the marked handles (during the framework's registry-handle-based view, which survives
    // Run_Restore's registry.clear() — unlike a cached _TransientEntity entity-id). DoTick then spawns + re-bridges
    // + clears the marker AFTER the view iteration, resolving the world per-entity from the live restored entity.
    //
    // Runs in FGroup_Gameplay_Script so the EntityReplicationDriver re-establish (Request_RebindActor ->
    // OwningActor::Add -> TryAdd -> Request_CreateEntity for replicated entities) happens in the SAME
    // scheduler-tick context as normal WithActor construction — NOT the FTSTicker, which crashed for replicated
    // entities (M2b-1 deviation #4).
    class CKECSEXT_API FProcessor_ActorRespawn : public TProcessor<
            FProcessor_ActorRespawn,
            ck::TReadOnly<FFragment_ActorSpawnIntent>>
    {
    public:
        using Group = FGroup_Gameplay_Script;

    public:
        using Super = TProcessor;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ActorSpawnIntent& InIntent) -> void;

    private:
        // Re-issue the entity-script REPLICATION for a restored+re-bridged entity. Request_RebindActor re-adds the
        // driver fragment (OwningActor::Add -> EntityReplicationDriver::TryAdd) but, unlike the fresh-spawn path
        // (FProcessor_EntityScript_Spawn), does NOT enqueue the replicate request that populates the driver's
        // ReplicationData_EntityScript. Without that payload the server's replicated driver object stays empty, so
        // clients receive the (independently-replicating) actor but never the ECS entity -> no bridge, no fragments.
        // Mirrors the fresh-spawn step, on the networked authority, for replicated entities only.
        auto
        DoReplicate_RestoredEntity(
            FCk_Handle& InEntity,
            AActor* InActor) -> void;

    private:
        // Filled by ForEachEntity (during the view iteration), consumed by DoTick (after it). Collect-then-act:
        // SpawnActor + the driver Request_CreateEntity mutate the registry, so they must not run mid-view.
        TArray<FCk_Handle> _PendingRespawns;
    };
}

// --------------------------------------------------------------------------------------------------------------------
