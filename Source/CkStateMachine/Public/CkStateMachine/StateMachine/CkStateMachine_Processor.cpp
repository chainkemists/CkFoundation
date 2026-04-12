#include "CkStateMachine_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_EndPlay);

static auto
    GetCleanClassName(
        const UClass* InClass)
    -> FString
{
    if (NOT IsValid(InClass))
    { return TEXT("(unknown)"); }

    auto Name = InClass->GetName();
    Name.RemoveFromStart(TEXT("BP_"));
    Name.RemoveFromEnd(TEXT("_C"));
    return Name;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // SETUP
    // ================================================================================================================

    auto
        FProcessor_Sm_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent)
        -> void
    {
        InHandle.Remove<FTag_Sm_RequiresSetup>();

        if (NOT InHandle.Has<FFragment_Sm_Context>())
        {
            auto OwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
            InHandle.Add<FFragment_Sm_Context>(OwnerEntity);
        }

        if (InParams.Get_AutoStart() == ECk_SmAutoStart::OnSetup)
        {
            auto& Requests = InHandle.AddOrGet<FFragment_Sm_Requests>();
            Requests._Requests.Add(FCk_Request_Sm_Start{});
        }
    }

    // ================================================================================================================
    // HANDLE REQUESTS
    // ================================================================================================================

    auto
        FProcessor_Sm_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FFragment_Sm_Requests& InRequests) const
        -> void
    {
        InHandle.CopyAndRemove(InRequests, [&](FFragment_Sm_Requests& InRequestsCopy)
        {
            algo::ForEachRequest(InRequestsCopy._Requests, Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Start& InRequest)
        -> void
    {
        if (InCurrent._RunStatus == ECk_SmRunStatus::Running)
        { return; }

        InCurrent._RunStatus = ECk_SmRunStatus::Running;
        InHandle.Add<FTag_Sm_Running>();
        InHandle.Try_Remove<FTag_Sm_Paused>();

        DoEnterState(InHandle, InCurrent, InParams.Get_InitialStateClass());

        UUtils_Signal_OnSmStarted::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Sm_Payload_OnStarted{}));

#if !UE_BUILD_SHIPPING
        if (InHandle.Has<FFragment_Sm_Breakpoints>())
        {
            const auto& Breakpoints = InHandle.Get<FFragment_Sm_Breakpoints>();

            if (Breakpoints.Get_EntryBreakpoints().Contains(InParams.Get_InitialStateClass()))
            {
                auto& HitFrag = InHandle.AddOrGet<FFragment_Sm_Debug_BreakpointHit>();
                HitFrag.Description = TEXT("Entry: ") + GetCleanClassName(InParams.Get_InitialStateClass());
                HitFrag.RealTimeSeconds = FPlatformTime::Seconds();
                UCk_Utils_EditorOnly_UE::Request_DebugPauseExecution();
            }
        }
#endif
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Stop& InRequest)
        -> void
    {
        if (InCurrent._RunStatus == ECk_SmRunStatus::Stopped)
        { return; }

        DoExitCurrentState(InHandle, InCurrent);

        InCurrent._RunStatus = ECk_SmRunStatus::Stopped;
        InHandle.Try_Remove<FTag_Sm_Running>();
        InHandle.Try_Remove<FTag_Sm_Paused>();
        InHandle.Try_Remove<FTag_Sm_TransitionQueued>();

        UUtils_Signal_OnSmStopped::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Sm_Payload_OnStopped{}));
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Pause& InRequest)
        -> void
    {
        if (InCurrent._RunStatus != ECk_SmRunStatus::Running)
        { return; }

        InCurrent._RunStatus = ECk_SmRunStatus::Paused;
        InHandle.Add<FTag_Sm_Paused>();
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Resume& InRequest)
        -> void
    {
        if (InCurrent._RunStatus != ECk_SmRunStatus::Paused)
        { return; }

        InCurrent._RunStatus = ECk_SmRunStatus::Running;
        InHandle.Remove<FTag_Sm_Paused>();
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Transition& InRequest)
        -> void
    {
        if (InCurrent._RunStatus != ECk_SmRunStatus::Running)
        { return; }

        const auto PreviousStateClass = InCurrent._CurrentStateClass;

#if !UE_BUILD_SHIPPING
        if (InHandle.Has<FFragment_Sm_Breakpoints>())
        {
            const auto& Breakpoints = InHandle.Get<FFragment_Sm_Breakpoints>();

            if (Breakpoints.Get_ExitBreakpoints().Contains(PreviousStateClass))
            {
                auto& HitFrag = InHandle.AddOrGet<FFragment_Sm_Debug_BreakpointHit>();
                HitFrag.Description = TEXT("Exit: ") + GetCleanClassName(PreviousStateClass);
                HitFrag.RealTimeSeconds = FPlatformTime::Seconds();
                UCk_Utils_EditorOnly_UE::Request_DebugPauseExecution();
            }
        }
#endif

        DoExitCurrentState(InHandle, InCurrent);
        DoEnterState(InHandle, InCurrent, InRequest.Get_TargetStateClass());

        InHandle.Try_Remove<FTag_Sm_TransitionQueued>();

        UUtils_Signal_OnSmStateChanged::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Sm_Payload_OnStateChanged{
                PreviousStateClass,
                InRequest.Get_TargetStateClass(),
                InCurrent._CurrentStateHandle
            }));

#if !UE_BUILD_SHIPPING
        if (InHandle.Has<FFragment_Sm_Breakpoints>())
        {
            const auto& Breakpoints = InHandle.Get<FFragment_Sm_Breakpoints>();

            if (Breakpoints.Get_EntryBreakpoints().Contains(InRequest.Get_TargetStateClass()))
            {
                auto& HitFrag = InHandle.AddOrGet<FFragment_Sm_Debug_BreakpointHit>();
                HitFrag.Description = TEXT("Entry: ") + GetCleanClassName(InRequest.Get_TargetStateClass());
                HitFrag.RealTimeSeconds = FPlatformTime::Seconds();
                UCk_Utils_EditorOnly_UE::Request_DebugPauseExecution();
            }
        }
#endif
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sm_HandleRequests::
        DoEnterState(
            HandleType InSmHandle,
            FFragment_Sm_Current& InCurrent,
            TSubclassOf<UCk_SmState_EntityScript> InStateClass)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InStateClass),
            TEXT("Invalid state class when entering state on SM [{}]"), InSmHandle)
        { return; }

        auto StateEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InSmHandle);

        UCk_Utils_Handle_UE::Set_DebugName(StateEntity, InStateClass->GetFName());

        if (InSmHandle.Has<FFragment_Sm_Context>())
        {
            const auto& Context = InSmHandle.Get<FFragment_Sm_Context>();
            StateEntity.Add<FFragment_Sm_Context>(Context.Get_GameEntityHandle());
        }

        auto StateEntityTyped = ck::StaticCast<FCk_Handle_SmState>(StateEntity);
        UCk_Utils_StateMachine_UE::RecordOfSmStates_Utils::AddIfMissing(InSmHandle);
        UCk_Utils_StateMachine_UE::RecordOfSmStates_Utils::Request_Connect(
            InSmHandle, StateEntityTyped, ECk_Record_LabelRequirementPolicy::Optional);

        TUtils_Sm_OwningStateMachine::AddOrReplace(StateEntity, InSmHandle);

        // Default every new state to event-driven — mirrors RelicSim SimCompState.cpp:43.
        // Call MarkStateAs_Ticking() in DefineState() to opt in to per-pump polling.
        StateEntity.Add<FTag_SmState_EventDriven>();

        auto PostConstructionFunc = [&InCurrent](FCk_Handle InStateEntity)
        {
            InCurrent._CurrentStateHandle = ck::StaticCast<FCk_Handle_SmState>(InStateEntity);
        };

        InCurrent._CurrentStateClass = InStateClass;

        UCk_Utils_EntityScript_UE::Add(
            StateEntity,
            InStateClass,
            FInstancedStruct{},
            PostConstructionFunc);
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoExitCurrentState(
            HandleType InSmHandle,
            FFragment_Sm_Current& InCurrent)
        -> void
    {
        if (ck::Is_NOT_Valid(InCurrent._CurrentStateHandle))
        { return; }

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InCurrent._CurrentStateHandle);

        InCurrent._CurrentStateHandle = FCk_Handle_SmState{};
        InCurrent._CurrentStateClass = nullptr;
    }

    // ================================================================================================================
    // ENDPLAY
    // ================================================================================================================

    auto
        FProcessor_Sm_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sm_Current& InCurrent)
        -> void
    {
        InCurrent._RunStatus = ECk_SmRunStatus::Stopped;
        InCurrent._CurrentStateHandle = FCk_Handle_SmState{};
        InCurrent._CurrentStateClass = nullptr;
    }
}

// --------------------------------------------------------------------------------------------------------------------
