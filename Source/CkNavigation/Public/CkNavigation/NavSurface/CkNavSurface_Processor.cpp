#include "CkNavigation/NavSurface/CkNavSurface_Processor.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/NavSurface/CkNavSurface_ProviderTable.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_NavSurfaceMarkup_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NavSurfaceMarkup_Requests& InRequests,
            FFragment_NavSurfaceMarkup_Current& InCurrent) const
        -> void
    {
        InHandle.CopyAndRemove(InRequests, [&](const FFragment_NavSurfaceMarkup_Requests& InSnapshot)
        {
            ck::algo::ForEachRequest(InSnapshot._Requests, ck::Visitor(
                [&](const auto& InRequest) -> void
                {
                    auto Result = ECk_Request_OperationResult::Failed;
                    const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                    Result = DoHandleRequest(InHandle, InRequest);
                }), ck::policy::DontResetContainer{});
        });
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_NavSurfaceMarkup_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_NavSurface_AreaMarkup& InRequest)
        -> ECk_Request_OperationResult
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        const auto Provider = nav_surface::Get_ProviderForWorld(World);
        const auto* Table = nav_surface::TryGet_ProviderTable(Provider);

        const auto ProviderCanAnswer = Table != nullptr;
        CK_ENSURE_IF_NOT(ProviderCanAnswer,
            TEXT("NavSurface markup on [{}] cannot be applied: provider [{}] has registered no capability table"),
            InHandle, Provider)
        { return ECk_Request_OperationResult::Failed; }

        auto GenericHandle = static_cast<FCk_Handle>(InHandle);

        return Table->_ApplyAreaMarkup(World, GenericHandle, InRequest)
            ? ECk_Request_OperationResult::Succeeded
            : ECk_Request_OperationResult::Failed;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_NavSurfaceMarkup_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_NavSurfaceMarkup_Requests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequests.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_NavSurfaceMarkup_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NavSurfaceMarkup_Current& InCurrent)
        -> void
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        const auto Provider = nav_surface::Get_ProviderForWorld(World);
        const auto* Table = nav_surface::TryGet_ProviderTable(Provider);

        const auto ProviderCanAnswer = Table != nullptr;
        CK_ENSURE_IF_NOT(ProviderCanAnswer,
            TEXT("NavSurface markup on [{}] cannot be released: provider [{}] has registered no capability table"),
            InHandle, Provider)
        { return; }

        auto GenericHandle = static_cast<FCk_Handle>(InHandle);

        Table->_ReleaseAreaMarkup(World, GenericHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_NavSurface_LinkTraversal_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NavSurface_LinkTraversal_Requests& InRequests,
            FFragment_NavSurface_LinkTraversal_Current& InCurrent) const
        -> void
    {
        InHandle.CopyAndRemove(InRequests, [&](const FFragment_NavSurface_LinkTraversal_Requests& InSnapshot)
        {
            ck::algo::ForEachRequest(InSnapshot._Requests, ck::Visitor(
                [&](const auto& InRequest) -> void
                {
                    auto Result = ECk_Request_OperationResult::Failed;
                    const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                    Result = DoHandleRequest(InHandle, InCurrent, InRequest);
                }), ck::policy::DontResetContainer{});
        });
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_NavSurface_LinkTraversal_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_NavSurface_LinkTraversal_Current& InCurrent,
            const FCk_Request_NavSurface_BeginLinkTraversal& InRequest)
        -> ECk_Request_OperationResult
    {
        if (InCurrent._State == ECk_NavSurface_LinkTraversalState::Traversing)
        {
            // The crossing the caller asked for is the one already running, so its intent holds and
            // there is nothing to report: a second Begun would break the one-per-crossing count.
            return InCurrent._ActiveCorrelatorId == InRequest.Get_CorrelatorId()
                ? ECk_Request_OperationResult::Succeeded
                : ECk_Request_OperationResult::Failed;
        }

        InCurrent._ActiveLinkId = InRequest.Get_LinkId();
        InCurrent._ActiveCorrelatorId = InRequest.Get_CorrelatorId();
        InCurrent._EntryDirection = InRequest.Get_EntryDirection();
        InCurrent._State = ECk_NavSurface_LinkTraversalState::Traversing;

        InHandle.AddOrGet<FTag_NavSurface_LinkTraversal_Active>();

        UUtils_Signal_NavSurface_OnLinkTraversalBegun::Broadcast(
            InHandle, ck::MakePayload(InHandle, InCurrent._ActiveLinkId, InCurrent._ActiveCorrelatorId));

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_NavSurface_LinkTraversal_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_NavSurface_LinkTraversal_Current& InCurrent,
            const FCk_Request_NavSurface_CompleteLinkTraversal& InRequest)
        -> ECk_Request_OperationResult
    {
        const auto CorrelatorIsActive = InCurrent._State == ECk_NavSurface_LinkTraversalState::Traversing &&
            InCurrent._ActiveCorrelatorId == InRequest.Get_CorrelatorId();

        // Not a no-op the caller can ignore: completing a crossing nobody is on means the caller's
        // model of which crossing it is driving is wrong, and retrying will not fix that.
        if (NOT CorrelatorIsActive)
        { return ECk_Request_OperationResult::Failed; }

        DoEnd_Traversal(InHandle, InCurrent, ECk_Request_OperationResult::Succeeded);

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_NavSurface_LinkTraversal_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_NavSurface_LinkTraversal_Current& InCurrent,
            const FCk_Request_NavSurface_CancelLinkTraversal& InRequest)
        -> ECk_Request_OperationResult
    {
        const auto CorrelatorIsActive = InCurrent._State == ECk_NavSurface_LinkTraversalState::Traversing &&
            InCurrent._ActiveCorrelatorId == InRequest.Get_CorrelatorId();

        // Cancelling a crossing that is already over leaves the caller's intent holding afterwards,
        // which is what Succeeded means.
        if (NOT CorrelatorIsActive)
        { return ECk_Request_OperationResult::Succeeded; }

        DoEnd_Traversal(InHandle, InCurrent, ECk_Request_OperationResult::Failed_Cancelled);

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_NavSurface_LinkTraversal_HandleRequests::
        DoEnd_Traversal(
            HandleType InHandle,
            FFragment_NavSurface_LinkTraversal_Current& InCurrent,
            ECk_Request_OperationResult InResult)
        -> void
    {
        const auto LinkId = InCurrent._ActiveLinkId;
        const auto CorrelatorId = InCurrent._ActiveCorrelatorId;

        InCurrent._ActiveLinkId = INDEX_NONE;
        InCurrent._ActiveCorrelatorId = INDEX_NONE;
        InCurrent._State = ECk_NavSurface_LinkTraversalState::None;

        InHandle.Try_Remove<FTag_NavSurface_LinkTraversal_Active>();

        // Broadcast LAST, with the state already cleared: a listener that begins the next crossing from
        // inside this one's end must find the traverser free.
        UUtils_Signal_NavSurface_OnLinkTraversalCompleted::Broadcast(
            InHandle, ck::MakePayload(InHandle, LinkId, CorrelatorId, InResult));
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_NavSurface_LinkTraversal_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NavSurface_LinkTraversal_Current& InCurrent)
        -> void
    {
        if (InHandle.Has<FFragment_NavSurface_LinkTraversal_Requests>())
        {
            request::FireCancelledForPending(InHandle,
                InHandle.Get<FFragment_NavSurface_LinkTraversal_Requests>().Get_Requests());
        }

        if (InCurrent._State != ECk_NavSurface_LinkTraversalState::Traversing)
        { return; }

        FProcessor_NavSurface_LinkTraversal_HandleRequests::DoEnd_Traversal(
            InHandle, InCurrent, ECk_Request_OperationResult::Failed_Cancelled);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_NavSurface_RevisionWatch::
        DoTick(
            FCk_Time InDeltaT)
        -> void
    {
        if (ck::IsValid(this->_TransientEntity, ck::IsValid_Policy_IncludePendingKill{}))
        {
            // Seeded on the tick that ADDS the fragment and never again: after that the value is the
            // world's own choice, and re-seeding it every tick would silently undo Request_SetProvider.
            const auto ProviderWasAlreadyAdded = this->_TransientEntity.Has<FFragment_NavSurface_Provider>();

            auto& Provider = this->_TransientEntity.AddOrGet<FFragment_NavSurface_Provider>();

            if (NOT ProviderWasAlreadyAdded)
            {
                const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(this->_TransientEntity);

                Provider._Provider = nav_surface::Get_DefaultProvider();
                Provider._ShadowMode = nav_surface::Get_DefaultShadowMode();

                nav_surface::Set_ProviderForWorld(World, Provider._Provider);
                nav_surface::Set_ShadowModeForWorld(World, Provider._ShadowMode);
            }

            this->_TransientEntity.AddOrGet<FFragment_NavSurface_RevisionWatch>();
            this->_TransientEntity.AddOrGet<FFragment_NavSurface_PendingRebuilds>();
        }

        TProcessor::DoTick(InDeltaT);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_NavSurface_RevisionWatch::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NavSurface_Provider& InProvider,
            FFragment_NavSurface_RevisionWatch& InWatch,
            FFragment_NavSurface_PendingRebuilds& InPending)
        -> void
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        const auto* Table = nav_surface::TryGet_ProviderTable(InProvider.Get_Provider());

        // A provider nobody registered has no health to report and no revision to compare against, so
        // it reads back its own last broadcast and the poll below finds nothing moved. What a provider
        // already PUSHED still drains: that is a rebuild it observed, not a query put to it now.
        InProvider._Health = Table != nullptr
            ? Table->_ProviderHealth(World)
            : ECk_NavSurface_ProviderHealth::NoData;

        const auto Revision = Table != nullptr
            ? Table->_SurfaceRevision(World)
            : InWatch.Get_LastBroadcastRevision();

        // Two providers' counters are unrelated numbers, so the one just switched to is ADOPTED rather
        // than compared against what the previous one had reached. Without this the poll below reads the
        // gap between them as a move and broadcasts bounds-unknown, which every consumer takes to mean
        // the whole surface changed. What a switch changes is which surface answers, not the surface.
        const auto CurrentProvider = InProvider.Get_Provider();
        const auto ProviderChanged = NOT InWatch._LastProvider.IsSet() ||
            InWatch._LastProvider.GetValue() != CurrentProvider;

        if (ProviderChanged)
        {
            InWatch._LastBroadcastRevision = Revision;
            InWatch._LastProvider = CurrentProvider;
        }

        // Still drained on the tick of a switch: the queue holds regions a provider OBSERVED it rebuilt,
        // and those happened whether or not the world has since changed who it asks.
        if (DoBroadcast_PendingRebuilds(InHandle, InWatch, InPending, Revision))
        { return; }

        DoBroadcast_RevisionPoll(InHandle, InWatch, Revision);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_NavSurface_RevisionWatch::
        DoBroadcast_PendingRebuilds(
            HandleType InHandle,
            FFragment_NavSurface_RevisionWatch& InWatch,
            FFragment_NavSurface_PendingRebuilds& InPending,
            int64 InRevision)
        -> bool
    {
        if (InPending._Bounds.IsEmpty())
        { return false; }

        // Taken off the fragment before the first broadcast: a listener that publishes from inside one
        // appends to the emptied list and is drained next tick, rather than being consumed mid-loop.
        const auto DrainedBounds = MoveTemp(InPending._Bounds);

        // The revision the queue is now caught up to. Without this, the poll below would read the same
        // provider move a second time and report it again as bounds-unknown.
        InWatch._LastBroadcastRevision = InRevision;

        for (const auto& ChangedBounds : DrainedBounds)
        {
            UUtils_Signal_NavSurface_OnSurfaceRebuilt::Broadcast(
                InHandle, ck::MakePayload(InHandle, ChangedBounds));
        }

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_NavSurface_RevisionWatch::
        DoBroadcast_RevisionPoll(
            HandleType InHandle,
            FFragment_NavSurface_RevisionWatch& InWatch,
            int64 InRevision)
        -> void
    {
        if (InRevision == InWatch.Get_LastBroadcastRevision())
        { return; }

        InWatch._LastBroadcastRevision = InRevision;

        UUtils_Signal_NavSurface_OnSurfaceRebuilt::Broadcast(
            InHandle, ck::MakePayload(InHandle, FBox{ForceInit}));
    }
}

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_NavSurfaceMarkup_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_NavSurfaceMarkup_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_NavSurfaceMarkup_EndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_NavSurface_LinkTraversal_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_NavSurface_LinkTraversal_EndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_NavSurface_RevisionWatch);

// --------------------------------------------------------------------------------------------------------------------
