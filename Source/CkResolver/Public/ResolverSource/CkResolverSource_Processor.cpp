#include "CkResolverSource_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkNet/CkNet_Utils.h"

#include "ResolverDataBundle/CkResolverDataBundle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ResolverSource_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_ResolverSource_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const ck::FFragment_ResolverSource_Params& InParams,
            FFragment_ResolverSource_Requests& InRequests) const
        -> void
    {
        ck::algo::ForEachRequest(InRequests._ResolverRequests,
        [&](const auto& InRequest)
        {
            DoHandleRequest(InHandle, InParams, InRequest);
        });
    }

    auto
        FProcessor_ResolverSource_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const ck::FFragment_ResolverSource_Params& InParams,
            const FCk_Request_ResolverSource_InitiateNewResolution& InNewResolution)
        -> void
    {
        auto Target = InNewResolution.Get_Target();

        auto DataBundle = UCk_Utils_ResolverDataBundle_UE::Create(InHandle, FCk_Fragment_ResolverDataBundle_ParamsData
            {
                InNewResolution.Get_BundleName(),
                InHandle,
                InNewResolution.Get_Target(),
                InNewResolution.Get_ResolverCause(),
                InParams.Get_Params().Get_ResolutionPhases()
            });

        UUtils_Signal_ResolverSource_OnNewResolverDataBundle::Broadcast(InNewResolution.GetAndDestroyRequestHandle(),
            MakePayload(InHandle, InNewResolution.Get_ResolverCause(), DataBundle));

        UUtils_Signal_ResolverSource_OnNewResolverDataBundle::Broadcast(InHandle,
            MakePayload(InHandle, InNewResolution.Get_ResolverCause(), DataBundle));
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
