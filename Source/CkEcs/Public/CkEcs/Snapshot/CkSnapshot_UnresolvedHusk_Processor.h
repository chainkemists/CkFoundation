#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h" // CK_IGNORE_PENDING_KILL
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h" // FTag_Snapshot_UnresolvedArchetype

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The route-agnostic half of the husk contract. A snapshot load reaps the husks IT produced at load finish
    // and names each one in its report; this resident processor reaps every husk that reaches a live world by
    // any OTHER route. Both exist because the marker records what an entity IS, not how it was made - the
    // feature's own construct guard stamps it - so a producer nobody has thought of still ends with a released
    // slot rather than a permanently dead one.
    //
    // Deliberately NOT declaring LoadPolicy (so the default GatedDuringLoad keeps it out of the load kernel)
    // and NOT declaring HydrationQuarantinePolicy (so its view excludes entities a load still owns). Between
    // them, a load's own husks belong to the load, and this processor never races it for them.
    class CKECS_API FProcessor_UnresolvedHusk_Reap : public ck_exp::TProcessor<
            FProcessor_UnresolvedHusk_Reap,
            FCk_Handle,
            FTag_Snapshot_UnresolvedArchetype,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;

        // Server-owned entities: a client husk is a mirror of one the authority still owns, so reaping it there
        // would delete half of a replicated pair and leave the other half publishing to nothing.
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

        // A load never runs in an editor world, so a husk there is an authoring preview of an unusable definition
        // rather than a restore failure - destroying it would delete what the designer is looking at.
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

        // The custom DoTick's only work outside the view is reading the load gate, which decides nothing when
        // no husk exists - so the scheduler may omit this processor entirely from an idle main pass.
        using MainPassRequiredFragments = entt::type_list<FTag_Snapshot_UnresolvedArchetype>;

        using Super = ck_exp::TProcessor<
            FProcessor_UnresolvedHusk_Reap,
            FCk_Handle,
            FTag_Snapshot_UnresolvedArchetype,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>;
        using TimeType = typename Super::TimeType;
        using HandleType = typename Super::HandleType;

    public:
        using Super::Super;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
