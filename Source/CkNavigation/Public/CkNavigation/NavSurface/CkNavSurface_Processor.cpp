#include "CkNavigation/NavSurface/CkNavSurface_Processor.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/NavAreaMarkup/CkNavAreaMarkup_Utils.h"
#include "CkNavigation/NavSurface/CkNavSurface_ProviderTable.h"
#include "CkNavigation/NavSurface/Recast/CkNavSurface_RecastAdapter.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include <NavAreas/NavArea.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_nav_surface_processor
{
    auto Get_HalfExtents(const FCk_AnyShape& InShape) -> FVector
    {
        switch (InShape.Get_ShapeType())
        {
            case ECk_Shape_Type::Box:
            {
                return InShape.Get_Box().Get_HalfExtents();
            }
            case ECk_Shape_Type::Capsule:
            {
                const auto Radius = static_cast<double>(InShape.Get_Capsule().Get_Radius());
                return FVector{Radius, Radius, static_cast<double>(InShape.Get_Capsule().Get_HalfHeight())};
            }
            case ECk_Shape_Type::Cylinder:
            {
                const auto Radius = static_cast<double>(InShape.Get_Cylinder().Get_Radius());
                return FVector{Radius, Radius, static_cast<double>(InShape.Get_Cylinder().Get_HalfHeight())};
            }
            case ECk_Shape_Type::Sphere:
            {
                return FVector{static_cast<double>(InShape.Get_Sphere().Get_Radius())};
            }
            default:
            {
                return FVector::ZeroVector;
            }
        }
    }

    auto DoUnpaint(ck::FFragment_NavSurfaceMarkup_Current& InCurrent) -> void
    {
        if (NOT InCurrent.Get_Markup().IsValid())
        { return; }

        UCk_Utils_NavAreaMarkup_UE::Request_Destroy(InCurrent.Get_Markup().Get());
    }
}

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

                    Result = DoHandleRequest(InHandle, InCurrent, InRequest);
                }), ck::policy::DontResetContainer{});
        });
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_NavSurfaceMarkup_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_NavSurfaceMarkup_Current& InCurrent,
            const FCk_Request_NavSurface_AreaMarkup& InRequest)
        -> ECk_Request_OperationResult
    {
        ck_nav_surface_processor::DoUnpaint(InCurrent);
        InCurrent._Markup = nullptr;

        if (InRequest.Get_Enable() == ECk_EnableDisable::Disable)
        {
            InCurrent._AreaTag = {};
            InCurrent._Location = FVector::ZeroVector;
            InCurrent._HalfExtents = FVector::ZeroVector;
            return ECk_Request_OperationResult::Succeeded;
        }

        const auto AreaClass = ck::nav_surface_recast::Get_AreaClass(InRequest.Get_AreaTag());
        const auto AreaClassIsValid = ck::IsValid(AreaClass.Get());
        CK_ENSURE_IF_NOT(AreaClassIsValid,
            TEXT("NavSurface markup on [{}] asked for area tag [{}], which no provider area is registered for"),
            InHandle, InRequest.Get_AreaTag())
        { return ECk_Request_OperationResult::Failed; }

        const auto HalfExtents = ck_nav_surface_processor::Get_HalfExtents(InRequest.Get_Shape());
        const auto ShapeIsPaintable = NOT HalfExtents.IsNearlyZero();
        CK_ENSURE_IF_NOT(ShapeIsPaintable,
            TEXT("NavSurface markup on [{}] was given a shape [{}] with no extent"),
            InHandle, InRequest.Get_Shape().Get_ShapeType())
        { return ECk_Request_OperationResult::Failed; }

        auto GenericHandle = static_cast<FCk_Handle>(InHandle);
        auto* Markup = UCk_Utils_NavAreaMarkup_UE::Request_Create(
            GenericHandle,
            InRequest.Get_WorldTransform(),
            HalfExtents,
            AreaClass);

        const auto MarkupIsValid = ck::IsValid(Markup);
        CK_ENSURE_IF_NOT(MarkupIsValid,
            TEXT("NavSurface markup on [{}] failed to register its nav-area painter"), InHandle)
        { return ECk_Request_OperationResult::Failed; }

        InCurrent._Markup = Markup;
        InCurrent._AreaTag = InRequest.Get_AreaTag();
        InCurrent._Location = InRequest.Get_WorldTransform().GetLocation();
        InCurrent._HalfExtents = HalfExtents;

        return ECk_Request_OperationResult::Succeeded;
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
        ck_nav_surface_processor::DoUnpaint(InCurrent);
        InCurrent._Markup = nullptr;
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
                Provider._Provider = nav_surface::Get_DefaultProvider();

                nav_surface::Set_ProviderForWorld(
                    UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(this->_TransientEntity), Provider._Provider);
            }

            this->_TransientEntity.AddOrGet<FFragment_NavSurface_RevisionWatch>();
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
            FFragment_NavSurface_RevisionWatch& InWatch)
        -> void
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        const auto* Table = nav_surface::TryGet_ProviderTable(InProvider.Get_Provider());

        if (Table == nullptr)
        {
            // A provider nobody registered has no health to report and no revision to compare against,
            // so the watch stays where it is rather than broadcasting a rebuild that never happened.
            InProvider._Health = ECk_NavSurface_ProviderHealth::NoData;
            return;
        }

        InProvider._Health = Table->_ProviderHealth(World);

        const auto Revision = Table->_SurfaceRevision(World);
        if (Revision == InWatch.Get_LastBroadcastRevision())
        { return; }

        InWatch._LastBroadcastRevision = Revision;
        UUtils_Signal_NavSurface_OnSurfaceRebuilt::Broadcast(InHandle, ck::MakePayload(InHandle));
    }
}

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_NavSurfaceMarkup_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_NavSurfaceMarkup_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_NavSurfaceMarkup_EndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_NavSurface_RevisionWatch);

// --------------------------------------------------------------------------------------------------------------------
