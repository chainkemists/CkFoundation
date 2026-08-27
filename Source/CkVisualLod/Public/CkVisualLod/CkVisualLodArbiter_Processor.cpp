#include "CkVisualLodArbiter_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkResourceLoader/CkResourceLoader_Utils.h"

#include "CkVisualLod/CkVisualLod_Log.h"
#include "CkVisualLod/CkVisualLodArbiter_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_VisualLodArbiter_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_VisualLodArbiter_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VisualLodArbiter_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_VisualLodArbiter_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VisualLodArbiter_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLodArbiter_Params& InParams,
            FFragment_VisualLodArbiter_Current& InCurrent)
        -> void
    {
        if (NOT InCurrent._LoadedAssets.Get_IsRequested())
        {
            InCurrent._LoadedAssets = UCk_Utils_ResourceLoader_UE::RequestLoad_RootedBatch(
                TEXT("VisualLodArbiter.Setup"), {InParams.Get_Config().ToSoftObjectPath()});
        }

        if (NOT InCurrent._LoadedAssets.Get_IsReady())
        {
            InHandle.AddOrGet<FTag_VisualLodArbiter_PendingAssetLoad>();
            return;
        }

        const auto ResolvedConfig = Cast<UCk_VisualLodArbiter_Data>(
            InCurrent._LoadedAssets.Get_ResolvedObject(InParams.Get_Config().ToSoftObjectPath()));
        const auto AssetsAreLoaded = NOT InCurrent._LoadedAssets.Get_HasFailed() && ck::IsValid(ResolvedConfig);

        CK_ENSURE_IF_NOT(AssetsAreLoaded,
            TEXT("Cannot setup VisualLodArbiter [{}] - loading its Config [{}] through CkResourceLoader failed"),
            InHandle, InParams.Get_Config().ToSoftObjectPath())
        {
            InCurrent._LoadedAssets = {};
            InHandle.Try_Remove<FTag_VisualLodArbiter_PendingAssetLoad>();
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        InCurrent._Config = ResolvedConfig;
        InCurrent._Crowds.SetNum(ResolvedConfig->Get_CrowdConfigs().Num());

        InHandle.Try_Remove<FTag_VisualLodArbiter_PendingAssetLoad>();
        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VisualLodArbiter_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            FFragment_VisualLodArbiter_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

            DoHandleRequest(InHandle, InCurrent, InRequest);

            Result = ECk_Request_OperationResult::Succeeded;
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        { InHandle.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_VisualLodArbiter_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            const FCk_Request_VisualLodArbiter_SetObserver& InRequest)
        -> void
    {
        InCurrent._Observer = InRequest.Get_Observer();
    }

    auto
        FProcessor_VisualLodArbiter_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            const FCk_Request_VisualLodArbiter_ClearObserver& InRequest)
        -> void
    {
        InCurrent._Observer = FCk_Handle{};
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VisualLodArbiter_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLodArbiter_Params& InParams,
            FFragment_VisualLodArbiter_Current& InCurrent)
        -> void
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VisualLodArbiter_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLodArbiter_Requests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequests.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
