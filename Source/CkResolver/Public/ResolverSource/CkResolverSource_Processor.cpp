#include "CkResolverSource_Processor.h"

#include "CkResolverTarget_Fragment_Data.h"
#include "CkResolverTarget_Utils.h"

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
    }

    auto
        FProcessor_ResolverSource_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const ck::FFragment_ResolverSource_Params& InParams,
            FFragment_ResolverSource_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_ResolverSource_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests._ResolverRequests,
            [&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InRequest);
            });
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
                Target,
                InNewResolution.Get_ResolverCause(),
                InParams.Get_Params().Get_ResolutionPhases()
            });

        UUtils_Signal_ResolverSource_OnNewResolverDataBundle::Broadcast(InNewResolution.GetAndDestroyRequestHandle(),
            MakePayload(InHandle, InNewResolution.Get_ResolverCause(), DataBundle));

        for (const auto& ModifierOperation : InNewResolution.Get_InitialModifierOperations())
        {
            UCk_Utils_ResolverDataBundle_UE::Request_AddOperation_Modifier(DataBundle,
                FRequest_ResolverDataBundle_ModifierOperation{}.Set_ModifierOperation(ModifierOperation));
        }

        for (const auto& MetadataOperation : InNewResolution.Get_InitialMetadata())
        {
            UCk_Utils_ResolverDataBundle_UE::Request_AddOperation_Metadata(DataBundle,
                FRequest_ResolverDataBundle_MetadataOperation{}.Set_MetadataOperation(MetadataOperation));
        }

        UUtils_Signal_ResolverSource_OnNewResolverDataBundle::Broadcast(InHandle,
            MakePayload(InHandle, InNewResolution.Get_ResolverCause(), DataBundle));

        UCk_Utils_ResolverTarget_UE::Request_InitiateNewResolution(Target, FCk_Request_ResolverTarget_InitiateNewResolution{DataBundle}, {});
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
