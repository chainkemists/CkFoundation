#include "CkEcs/Snapshot/CkSnapshot_UnresolvedHusk_Processor.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Payload/CkPayload.h" // ck::MakePayload
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_BuildRecipe.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Snapshot/CkSnapshot_UnresolvedHusk_Signals.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_UnresolvedHusk_Reap);

namespace ck_snapshot_unresolved_husk_processor
{
    // The archetype identity the recipe carries, read back off the recipe the build left on the entity. Empty
    // is a legitimate answer twice over: a husk minted by a route that keeps no recipe, and one whose recipe
    // named no archetype at all.
    auto
        DoGet_ArchetypeIdentityPath(
            const FCk_Handle& InHusk)
        -> FString
    {
        if (NOT InHusk.Has<ck::FFragment_BuildRecipe>())
        { return {}; }

        for (const auto& Info : InHusk.Get<ck::FFragment_BuildRecipe>().Get_ConstructionInfos())
        {
            if (const auto& Path = Info.Get_ArchetypeIdentityPath(); NOT Path.IsEmpty())
            { return Path; }
        }

        return {};
    }
}

namespace ck
{
    auto
        FProcessor_UnresolvedHusk_Reap::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        // A load OWNS the husks it produced: it reaps them at load finish and names each one in its report, and
        // a husk destroyed out from under it is a loss the player is never told about. GatedDuringLoad already
        // covers the ordinary phases, but an ESCALATED rebuild runs the FULL processor scope at zero time - this
        // is what keeps that escalation from turning into a silent reap.
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);
        const auto* EcsWorld = ck::IsValid(World) ? World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>() : nullptr;

        if (ck::IsValid(EcsWorld) && EcsWorld->Get_IsLoadGateActive())
        {
            _LastVisitedCount = 0;
            return;
        }

        Super::DoTick(InDeltaT);
    }

    auto
        FProcessor_UnresolvedHusk_Reap::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        // Re-asked per entity: NetModeRequirement answers the question ONCE, against the world's transient
        // entity, and a husk can carry a net mode of its own through its ownership chain. Spelled as the host
        // net-mode question rather than Get_HasAuthority for the reason CkNet's own AuthorityOnly branch is -
        // the authority tag is present on pure clients too, and an entity with no net info anywhere in its
        // chain is purely local and therefore ours.
        if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InHandle))
        { return; }

        const auto ArchetypePath = ck_snapshot_unresolved_husk_processor::DoGet_ArchetypeIdentityPath(InHandle);

        CK_TRIGGER_ENSURE(
            TEXT("Entity [{}] is an unresolved-archetype husk that reached a live world by a route the load ")
            TEXT("cannot account for: it was built from a class default, so it is structurally an entity and ")
            TEXT("semantically nothing, while still holding whatever slot or cell it occupies. Its recorded ")
            TEXT("archetype is [{}]. Every KNOWN producer is a snapshot load, which reaps its own at load ")
            TEXT("finish and NAMES each one in its report - so find the route that minted this one. Destroying ")
            TEXT("it through the ordinary path so the slot it holds is released"),
            InHandle, ArchetypePath);

        // Unconditional, and the reason it is not folded into the ensure: an ensure fires ONCE per site for the
        // life of the process, so past the first husk this line is the only per-occurrence record there is.
        ecs::Error(TEXT("Reaping unresolved-archetype husk entity [{}] with archetype [{}] - minted by a route that is not a load"),
            InHandle, ArchetypePath);

        UUtils_Signal_Snapshot_OnUnresolvedHuskReaped::Broadcast(_TransientEntity,
            ck::MakePayload(InHandle, ArchetypePath));

        // Through the ordinary destroy path, which is what releases the inventory record entry and grid cell a
        // husk item holds. A husk removed any other way frees the entity and leaves the container counting it.
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
