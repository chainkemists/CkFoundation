#include "CkStateMachine_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkStateMachine/CkStateMachine_Debug_Fragment.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/EntityScripts/CkSmTask_EntityScript.h"
#include "CkStateMachine/EntityScripts/CkSmCondition_EntityScript.h"

// --------------------------------------------------------------------------------------------------------------------

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

        auto SmHandle = static_cast<FCk_Handle>(InSmHandle);
        auto StateEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(SmHandle);

        if (SmHandle.Has<FFragment_Sm_Context>())
        {
            const auto& Context = SmHandle.Get<FFragment_Sm_Context>();
            StateEntity.Add<FFragment_Sm_Context>(Context.Get_GameEntityHandle());
        }

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
    // CONDITION RESET
    // ================================================================================================================

    auto
        FProcessor_SmCondition_ResetEveryFrame::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmCondition_Current& InCurrent)
        -> void
    {
        if (InCurrent._ResetBehavior != ECk_SmConditionResetBehavior::ResetEveryFrame)
        { return; }

        InCurrent._IsSatisfied = false;
    }

    // ================================================================================================================
    // CONDITION POLLED
    // ================================================================================================================

    auto
        FProcessor_SmCondition_Polled::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmCondition_Current& InCurrent)
        -> void
    {
        if (NOT InHandle.Has<FFragment_EntityScript_Current>())
        { return; }

        const auto& ScriptFragment = InHandle.Get<FFragment_EntityScript_Current>();
        auto* Script = ScriptFragment.Get_Script().Get();

        if (ck::Is_NOT_Valid(Script))
        { return; }

        auto* ConditionScript = Cast<UCk_SmCondition_EntityScript>(Script);
        if (ck::Is_NOT_Valid(ConditionScript))
        { return; }

        InCurrent._IsSatisfied = ConditionScript->Evaluate();
    }

    // ================================================================================================================
    // EVAL TRANSITIONS
    // ================================================================================================================

    auto
        FProcessor_Sm_EvalTransitions::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Current& InCurrent)
        -> void
    {
        auto StateHandle = static_cast<FCk_Handle>(InCurrent.Get_CurrentStateHandle());

        if (ck::Is_NOT_Valid(StateHandle))
        { return; }

        const auto StateDependents = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(StateHandle);

        struct FTransitionCandidate
        {
            FCk_Handle Handle;
            int32 Order;
            TSubclassOf<UCk_SmState_EntityScript> TargetStateClass;
        };

        auto Transitions = TArray<FTransitionCandidate>{};

        for (const auto& ChildHandle : StateDependents)
        {
            if (NOT ChildHandle.Has<FFragment_SmTransition_Params>())
            { continue; }

            const auto& TransitionParams = ChildHandle.Get<FFragment_SmTransition_Params>();
            Transitions.Add(FTransitionCandidate{
                ChildHandle,
                TransitionParams.Get_Order(),
                TransitionParams.Get_TargetStateClass()
            });
        }

        Transitions.Sort([](const FTransitionCandidate& A, const FTransitionCandidate& B)
        {
            return A.Order < B.Order;
        });

        for (const auto& Transition : Transitions)
        {
            if (NOT DoAreAllConditionsSatisfied(Transition.Handle))
            { continue; }

#if !UE_BUILD_SHIPPING
            if (InHandle.Has<FFragment_Sm_Breakpoints>())
            {
                const auto& Breakpoints = InHandle.Get<FFragment_Sm_Breakpoints>();
                auto TransitionKey = FFragment_Sm_Breakpoints::FTransitionKey{
                    InCurrent.Get_CurrentStateClass(), Transition.TargetStateClass};

                if (Breakpoints.Get_TransitionBreakpoints().Contains(TransitionKey))
                {
                    auto& HitFrag = InHandle.AddOrGet<FFragment_Sm_Debug_BreakpointHit>();
                    HitFrag.Description = TEXT("Transition: ")
                        + GetCleanClassName(InCurrent.Get_CurrentStateClass())
                        + TEXT(" \u2192 ")
                        + GetCleanClassName(Transition.TargetStateClass);
                    HitFrag.RealTimeSeconds = FPlatformTime::Seconds();
                    UCk_Utils_EditorOnly_UE::Request_DebugPauseExecution();
                }
            }
#endif

            auto& SmRequests = InHandle.AddOrGet<FFragment_Sm_Requests>();
            SmRequests._Requests.Add(FCk_Request_Sm_Transition{Transition.TargetStateClass});

            InHandle.Add<FTag_Sm_TransitionQueued>();

#if !UE_BUILD_SHIPPING
            {
                auto& LastFired = InHandle.AddOrGet<FFragment_Sm_Debug_LastFiredTransition>();
                LastFired.Order = Transition.Order;
                LastFired.RealTimeSeconds = FPlatformTime::Seconds();
                LastFired.ConditionNames.Reset();

                const auto CondDependents = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(Transition.Handle);

                for (const auto& CondHandle : CondDependents)
                {
                    if (NOT CondHandle.Has<FFragment_SmCondition_Current>())
                    { continue; }

                    if (CondHandle.Has<FFragment_EntityScript_Current>())
                    {
                        auto* CondScript = CondHandle.Get<FFragment_EntityScript_Current>().Get_Script().Get();

                        if (ck::IsValid(CondScript))
                        {
                            auto CondName = CondScript->GetClass()->GetName();
                            CondName.RemoveFromStart(TEXT("BP_"));
                            CondName.RemoveFromEnd(TEXT("_C"));
                            LastFired.ConditionNames.Add(MoveTemp(CondName));
                        }
                    }
                }
            }
#endif

            ck::sm::Verbose(TEXT("SM [{}] transition queued to [{}] (order [{}])"),
                InHandle, Transition.TargetStateClass->GetName(), Transition.Order);

            return;
        }
    }

    auto
        FProcessor_Sm_EvalTransitions::
        DoAreAllConditionsSatisfied(
            FCk_Handle InTransitionHandle)
        -> bool
    {
        const auto Dependents = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InTransitionHandle);

        auto HasAnyConditions = false;

        for (const auto& ChildHandle : Dependents)
        {
            if (NOT ChildHandle.Has<FFragment_SmCondition_Current>())
            { continue; }

            HasAnyConditions = true;

            const auto& ConditionCurrent = ChildHandle.Get<FFragment_SmCondition_Current>();
            if (NOT ConditionCurrent.Get_IsSatisfied())
            { return false; }
        }

        return HasAnyConditions;
    }

    // ================================================================================================================
    // TASK TICK
    // ================================================================================================================

    auto
        FProcessor_SmTask_Tick::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmTask_Current& InCurrent)
        -> void
    {
        if (NOT InHandle.Has<FFragment_EntityScript_Current>())
        { return; }

        const auto& ScriptFragment = InHandle.Get<FFragment_EntityScript_Current>();
        auto* Script = ScriptFragment.Get_Script().Get();

        if (ck::Is_NOT_Valid(Script))
        { return; }

        auto* TaskScript = Cast<UCk_SmTask_EntityScript>(Script);
        if (ck::Is_NOT_Valid(TaskScript))
        { return; }

        const auto Result = TaskScript->Tick(InDeltaT.Get_Seconds());
        InCurrent._LastResult = Result;
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
