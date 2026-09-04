#include "CkGroundNavPath_Invalidate_Processor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkGroundNav/CkGroundNav_Log.h"
#include "CkGroundNav/Debug/CkGroundNav_DebugGates.h"
#include "CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h"

#include "CkNavigation/NavSurface/CkNavSurface_Fragment.h"
#include "CkNavigation/NavSurface/CkNavSurface_ProviderTable.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavPath_InvalidateOnRebuilt);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_GroundNavPath_InvalidateOnRebuilt::
        DoTick(
            FCk_Time InDeltaT)
        -> void
    {
        _PublishedRebuilds = nullptr;

        // Written before every early-out below, so a world publishing nothing lets the pump go quiet
        // instead of scoring this pass as work the sentinel says it cannot account for.
        this->_LastVisitedCount = 0;

        if (groundnav::debug::Get_IsRepathOnRebuildBypassed())
        { return; }

        if (ck::Is_NOT_Valid(this->_TransientEntity, ck::IsValid_Policy_IncludePendingKill{}))
        { return; }

        // Composed by the watch on its first tick and by whoever publishes a rebuild. Its absence is a
        // world that has never published one, not a world whose queue is empty.
        if (NOT this->_TransientEntity.Has<FFragment_NavSurface_PendingRebuilds>())
        { return; }

        const auto& PublishedRebuilds =
            this->_TransientEntity.Get<FFragment_NavSurface_PendingRebuilds>().Get_Bounds();

        if (PublishedRebuilds.IsEmpty())
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(this->_TransientEntity);

        // Every provider publishes its rebuilds through the same queue, and a corridor cached here was
        // planned over a GroundNav field. What another provider rebuilt is not what moved this route.
        if (nav_surface::Get_ProviderForWorld(World) != ECk_NavSurface_Provider::GroundNav)
        { return; }

        _PublishedRebuilds = &PublishedRebuilds;

        TProcessor::DoTick(InDeltaT);

        _PublishedRebuilds = nullptr;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavPath_InvalidateOnRebuilt::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Current& InCurrent) const
        -> void
    {
        const auto& Corridor = InCurrent.Get_LastCorridorBounds();

        // An agent holding no corridor has no route a rebuild can have moved: its next plan reads the
        // field as it is, and a flag raised here would ask it to redo a plan it never made.
        if (Corridor.IsValid == 0)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InPathEntity);

        // The corridor's OWN profile rides the lookup. A route planned over a variant is answered by
        // that variant's field, which is the one its epoch dates it against; resolving the untagged
        // default here would measure a plan against ground it was never made on, and a change that
        // reached only the variant would read as no change at all.
        const auto Field = groundnav::world_fields::TryGet_Field(
            World, Corridor.GetCenter(), InCurrent.Get_ProfileTag());

        // A corridor found on the epoch the world publishes NOW already postdates every rebuild this
        // queue can describe, however many boxes one burst pushed - so a route planned between the
        // publish and this pass is left alone. A world with no field has no epoch to postdate, and the
        // boxes below are then the whole answer.
        const auto CorridorIsCurrent =
            Field.IsValid() && NOT Field->_Epoch.Get_IsNewerThan(InCurrent.Get_LastCorridorEpoch());

        if (CorridorIsCurrent)
        { return; }

        const auto PublishNote = groundnav::world_fields::TryGet_PublishNote(World, Corridor.GetCenter());

        // Narrowing needs the note to account for every publish THIS corridor has missed, not merely
        // for the newest one. What it accounts for is the run of link-only publishes since the last
        // geometry publish, so a plan made at or after that epoch has missed nothing a link id cannot
        // describe and the accumulated ids are the whole of what moved under it. A plan older than it
        // has missed ground moving - the repair half of a repair and a derive landing in one tick -
        // and takes the floor. The epochs are also compared because the note and the field are read
        // under two separate locks, and a note describing a publish this field is not the product of
        // accounts for nothing here. A world with no field has no epoch to agree with, and the boxes
        // are then the whole answer. A note is stamped from the entry's DEFAULT field, so a corridor
        // planned over a variant only ever agrees with it while nothing has moved either apart - which
        // is what drops a variant-only change to the boxes below instead of letting it narrow by link
        // identity.
        const auto NoteAccountsForEverythingSinceThePlan =
            PublishNote.IsSet() && Field.IsValid() &&
            PublishNote->_Epoch == Field->_Epoch &&
            NOT PublishNote->_LastGeometryEpoch.Get_IsNewerThan(InCurrent.Get_LastCorridorEpoch());

        if (NoteAccountsForEverythingSinceThePlan)
        {
            DoTry_FlagOnChangedLink(InPathEntity, InCurrent, *PublishNote);
            return;
        }

        for (const auto& RebuiltBounds : *_PublishedRebuilds)
        {
            // An invalid box is a publisher that did not know WHERE it rebuilt. Nothing can be ruled out
            // against it, so every cached corridor is news.
            const auto CorridorWasReached =
                RebuiltBounds.IsValid == 0 || RebuiltBounds.Intersect(Corridor);

            if (NOT CorridorWasReached)
            { continue; }

            InPathEntity.AddOrGet<FTag_GroundNavPath_RepathRequired>();

            groundnav::Verbose(
                TEXT("GroundNav Path [{}] flagged for repath: published rebuild [{}] meets corridor [{}]"),
                InPathEntity, RebuiltBounds, Corridor);

            return;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavPath_InvalidateOnRebuilt::
        DoTry_FlagOnChangedLink(
            HandleType                                                InPathEntity,
            const FFragment_GroundNavPath_Current&                    InCurrent,
            const groundnav::world_fields::FCk_GroundNav_PublishNote& InNote) const
        -> void
    {
        const auto& CorridorLinkIds = InCurrent.Get_LastCorridorLinkIds();

        for (const auto ChangedLinkId : InNote._ChangedLinkIdsSinceGeometry)
        {
            // Both lists are AUTHORED ids, so this comparison survives the renumbering of _ResolvedLinks
            // that the very publish being answered performed.
            if (NOT CorridorLinkIds.Contains(ChangedLinkId))
            { continue; }

            InPathEntity.AddOrGet<FTag_GroundNavPath_RepathRequired>();

            groundnav::Verbose(
                TEXT("GroundNav Path [{}] flagged for repath: a link-only publish since this plan moved ")
                TEXT("link [{}], which this corridor crosses"),
                InPathEntity, ChangedLinkId);

            return;
        }

        // A route that crosses none of the links these publishes moved is a route over ground that did
        // not move: a derive re-resolves links and re-labels and touches no geometry.
        groundnav::Verbose(
            TEXT("GroundNav Path [{}] left alone: the link-only publishes since this plan moved [{}] ")
            TEXT("link(s), none of the [{}] this corridor crosses"),
            InPathEntity, InNote._ChangedLinkIdsSinceGeometry.Num(), CorridorLinkIds.Num());
    }
}

// --------------------------------------------------------------------------------------------------------------------
