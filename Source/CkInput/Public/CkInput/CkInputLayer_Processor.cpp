#include "CkInputLayer_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Payload/CkPayload.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkInput/CkInputLayer_Utils.h"
#include "CkInput/CkInput_Log.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_InputLayer_SetupRouterState);
CK_REGISTER_PROCESSOR(ck::FProcessor_InputLayer_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_InputLayer_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_InputLayer_Route);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_InputLayer_SetupRouterState::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InInputSource,
            const FFragment_InputSource_Current& InCurrent)
        -> void
    {
        InInputSource.Add<FFragment_InputLayer_RouterState>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InputLayer_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InLayer,
            FFragment_InputLayer_Current& InCurrent,
            FFragment_InputLayer_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InLayer, Result);

            Result = DoHandleRequest(InLayer, InCurrent, InRequest);
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        {
            InLayer.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_InputLayer_HandleRequests::
        DoHandleRequest(
            HandleType InLayer,
            FFragment_InputLayer_Current& InCurrent,
            const FCk_Request_InputLayer_AddCapture& InRequest)
        -> ECk_Request_OperationResult
    {
        const auto& Capture = InRequest.Get_Capture();

        const auto ExistingIndex = InCurrent._Captures.IndexOfByPredicate(
        [&](const FCk_InputLayer_Capture& InExisting) -> bool
        {
            return InExisting.Get_MatchMode() == Capture.Get_MatchMode() && InExisting.Get_Key() == Capture.Get_Key();
        });

        if (ExistingIndex != INDEX_NONE)
        {
            InCurrent._Captures[ExistingIndex] = Capture;
        }
        else
        {
            InCurrent._Captures.Emplace(Capture);
        }

        input::VeryVerbose
        (
            TEXT("InputLayer [{}] now captures [{}|{}] with behavior [{}]"),
            InLayer, Capture.Get_MatchMode(), Capture.Get_Key(), Capture.Get_Behavior()
        );

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_InputLayer_HandleRequests::
        DoHandleRequest(
            HandleType InLayer,
            FFragment_InputLayer_Current& InCurrent,
            const FCk_Request_InputLayer_RemoveCapture& InRequest)
        -> ECk_Request_OperationResult
    {
        const auto MatchMode = InRequest.Get_MatchMode();
        const auto& Key = InRequest.Get_Key();

        // Removing a capture the layer never declared leaves the caller's intent holding, so it is a success.
        InCurrent._Captures.RemoveAll(
        [&](const FCk_InputLayer_Capture& InExisting) -> bool
        {
            return InExisting.Get_MatchMode() == MatchMode && InExisting.Get_Key() == Key;
        });

        input::VeryVerbose
        (
            TEXT("InputLayer [{}] no longer captures [{}|{}]"),
            InLayer, MatchMode, Key
        );

        return ECk_Request_OperationResult::Succeeded;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InputLayer_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InLayer,
            const FFragment_InputLayer_Requests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InLayer, InRequests.Get_Requests());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InputLayer_Route::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InInputSource,
            FFragment_InputSource_Current& InCurrent,
            FFragment_InputLayer_RouterState& InRouterState) const
        -> void
    {
        const auto EventsCopy = InCurrent._PendingRawEvents;
        InCurrent._PendingRawEvents.Reset();

        if (EventsCopy.IsEmpty())
        { return; }

        DoRankLayers(InInputSource, InRouterState);

        for (const auto& Event : EventsCopy)
        {
            DoRouteEvent(InInputSource, InRouterState, Event);
        }
    }

    auto
        FProcessor_InputLayer_Route::
        DoRankLayers(
            HandleType InInputSource,
            FFragment_InputLayer_RouterState& InRouterState)
        -> void
    {
        auto& RankedLayers = InRouterState._RankedLayers;
        RankedLayers.Reset();

        const auto Registry = InInputSource.Get_RegistryView();

        InInputSource.View<
            FFragment_InputLayer_Params,
            FFragment_InputLayer_Current,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>().ForEach(
            [&](
                FCk_Entity InEntity,
                const FFragment_InputLayer_Params& InParams,
                const FFragment_InputLayer_Current&)
            {
                if (InParams.Get_InputSource() != InInputSource)
                { return; }

                auto EntityHandle = ck::MakeHandle(InEntity, Registry);

                RankedLayers.Emplace(FInputLayer_RankedLayer
                {
                    UCk_Utils_InputLayer_UE::CastChecked(EntityHandle),
                    InParams.Get_Priority()
                });
            });

        algo::Sort(RankedLayers,
        [](const FInputLayer_RankedLayer& InLhs, const FInputLayer_RankedLayer& InRhs) -> bool
        {
            return InLhs.Get_Priority() > InRhs.Get_Priority();
        });
    }

    auto
        FProcessor_InputLayer_Route::
        DoRouteEvent(
            HandleType InInputSource,
            FFragment_InputLayer_RouterState& InRouterState,
            const FCk_InputSource_RawEvent& InEvent)
        -> void
    {
        const auto EventType = InEvent.Get_EventType();

        if (EventType == ECk_InputSource_EventType::Released)
        {
            const auto OwnerIndex = DoFindPressOwnerIndex(InRouterState, InEvent.Get_Key());

            if (OwnerIndex != INDEX_NONE)
            {
                DoConsumeOwnedRelease(InRouterState, OwnerIndex, InEvent);
                return;
            }
        }

        for (const auto& Ranked : InRouterState._RankedLayers)
        {
            const auto Layer = Ranked.Get_Layer();

            const auto LayerIsLive = ck::IsValid(Layer) && NOT UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(
                Layer, ECk_EntityLifetime_DestructionPhase::BeginDestroy);

            if (NOT LayerIsLive)
            { continue; }

            const auto& Captures = Layer.Get<FFragment_InputLayer_Current>().Get_Captures();

            const auto MatchIndex = Captures.IndexOfByPredicate(
            [&](const FCk_InputLayer_Capture& InCapture) -> bool
            {
                switch (InCapture.Get_MatchMode())
                {
                    case ECk_InputLayer_CaptureMatch::CatchAll:
                    { return true; }
                    case ECk_InputLayer_CaptureMatch::Key:
                    { return InCapture.Get_Key() == InEvent.Get_Key(); }
                    default:
                    {
                        CK_INVALID_ENUM(InCapture.Get_MatchMode());
                        return false;
                    }
                }
            });

            if (MatchIndex == INDEX_NONE)
            { continue; }

            const auto Capture = Captures[MatchIndex];

            DoDeliver(Layer, InEvent, Capture);

            if (Capture.Get_Behavior() == ECk_InputLayer_CaptureBehavior::PassThrough)
            { continue; }

            if (EventType == ECk_InputSource_EventType::Pressed)
            {
                const auto& Key = InEvent.Get_Key();

                InRouterState._PressOwners.RemoveAll(
                [&](const FInputLayer_PressOwnership& InOwnership) -> bool
                {
                    return InOwnership.Get_Key() == Key;
                });

                InRouterState._PressOwners.Emplace(FInputLayer_PressOwnership{Key, Layer, Capture});
            }

            return;
        }
    }

    auto
        FProcessor_InputLayer_Route::
        DoFindPressOwnerIndex(
            const FFragment_InputLayer_RouterState& InRouterState,
            const FKey& InKey)
        -> int32
    {
        return InRouterState.Get_PressOwners().IndexOfByPredicate(
        [&](const FInputLayer_PressOwnership& InOwnership) -> bool
        {
            return InOwnership.Get_Key() == InKey;
        });
    }

    auto
        FProcessor_InputLayer_Route::
        DoConsumeOwnedRelease(
            FFragment_InputLayer_RouterState& InRouterState,
            int32 InOwnerIndex,
            const FCk_InputSource_RawEvent& InEvent)
        -> void
    {
        const auto Ownership = InRouterState._PressOwners[InOwnerIndex];
        InRouterState._PressOwners.RemoveAt(InOwnerIndex);

        const auto Owner = Ownership.Get_Owner();

        const auto OwnerIsLive = ck::IsValid(Owner) && NOT UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(
            Owner, ECk_EntityLifetime_DestructionPhase::BeginDestroy);

        // A dead owner drops the release rather than passing it down: the layers below never saw the press, and a
        // key-up for a press they never received would leave them holding state no press ever opened.
        if (NOT OwnerIsLive)
        {
            input::VeryVerbose
            (
                TEXT("Release of [{}] belonged to InputLayer [{}], which is gone — the release is dropped rather "
                     "than offered to the layers below"),
                InEvent.Get_Key(), Owner
            );

            return;
        }

        DoDeliver(Owner, InEvent, Ownership.Get_Capture());
    }

    auto
        FProcessor_InputLayer_Route::
        DoDeliver(
            FCk_Handle_InputLayer InLayer,
            const FCk_InputSource_RawEvent& InEvent,
            const FCk_InputLayer_Capture& InCapture)
        -> void
    {
        input::VeryVerbose
        (
            TEXT("InputLayer [{}] captured [{}] on key [{}] with behavior [{}]"),
            InLayer, InEvent.Get_EventType(), InEvent.Get_Key(), InCapture.Get_Behavior()
        );

        UUtils_Signal_OnInputCaptureTriggered::Broadcast(InLayer, MakePayload(InLayer, InEvent, InCapture));
    }
}

// --------------------------------------------------------------------------------------------------------------------
