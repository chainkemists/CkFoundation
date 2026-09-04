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

        const auto Field = groundnav::world_fields::TryGet_Field(World, Corridor.GetCenter());

        // A corridor found on the epoch the world publishes NOW already postdates every rebuild this
        // queue can describe, however many boxes one burst pushed - so a route planned between the
        // publish and this pass is left alone. A world with no field has no epoch to postdate, and the
        // boxes below are then the whole answer.
        const auto CorridorIsCurrent =
            Field.IsValid() && NOT Field->_Epoch.Get_IsNewerThan(InCurrent.Get_LastCorridorEpoch());

        if (CorridorIsCurrent)
        { return; }

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
}

// --------------------------------------------------------------------------------------------------------------------
