#include "CkTimer_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"

#include "CkTimer/CkTimer_Fragment.h"
#include "CkTimer/CkTimer_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Timer_ParamsData& InParams)
    -> FCk_Handle_Timer
{
    auto NewTimerEntity = Conv_HandleToTimer
    (
        UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
        {
            UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_TimerName());

            InNewEntity.Add<ck::FFragment_Timer_Params>(InParams);
            InNewEntity.Add<ck::FFragment_Timer_Current>(FCk_Chrono{InParams.Get_Duration()});

            if (InParams.Get_CountDirection() == ECk_Timer_CountDirection::CountDown)
            { InNewEntity.Add<ck::FTag_Timer_Countdown>(); }

            if (InParams.Get_StartingState() == ECk_Timer_State::Running)
            {
                InNewEntity.Add<ck::FTag_Timer_NeedsUpdate>();
            }
        })
    );

    RecordOfTimers_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfTimers_Utils::Request_Connect(InHandle, NewTimerEntity);

    return Conv_HandleToTimer(NewTimerEntity);
}

auto
    UCk_Utils_Timer_UE::
    AddOrReplace(
        FCk_Handle InTimerOwnerEntity,
        const FCk_Fragment_Timer_ParamsData& InParams)
    -> FCk_Handle_Timer
{
    if (const auto& ExistingTimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<UCk_Utils_Timer_UE,
            RecordOfTimers_Utils>(InTimerOwnerEntity, InParams.Get_TimerName());
        ck::Is_NOT_Valid(ExistingTimerEntity))
    {
        return Add(InTimerOwnerEntity, InParams);
    }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InParams.Get_TimerName());

    TimerEntity.Replace<ck::FFragment_Timer_Params>(InParams);
    TimerEntity.Replace<ck::FFragment_Timer_Current>(FCk_Chrono{InParams.Get_Duration()});

    if (InParams.Get_StartingState() == ECk_Timer_State::Running)
    {
        TimerEntity.AddOrGet<ck::FTag_Timer_NeedsUpdate>();
    }

    return Conv_HandleToTimer(TimerEntity);
}

auto
    UCk_Utils_Timer_UE::
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleTimer_ParamsData& InParams)
    -> TArray<FCk_Handle_Timer>
{
    return ck::algo::Transform<TArray<FCk_Handle_Timer>>(InParams.Get_TimerParams(),
    [&](const FCk_Fragment_Timer_ParamsData& InTimerParams)
    {
        return Add(InHandle, InTimerParams);
    });
}

auto
    UCk_Utils_Timer_UE::Has(
        const FCk_Handle& InAbilityEntity)
        -> bool { return InAbilityEntity.Has_All<ck::FFragment_Timer_Params, ck::FFragment_Timer_Current>(); }

auto
    UCk_Utils_Timer_UE::Cast(
        const FCk_Handle&    InHandle,
        ECk_SucceededFailed& OutResult)
        -> FCk_Handle_Timer
{
    if (Has(InHandle))
    {
        OutResult = ECk_SucceededFailed::Failed;
        return {};
    }
    OutResult = ECk_SucceededFailed::Succeeded;
    return ck::Cast<FCk_Handle_Timer>(InHandle);
}

auto
    UCk_Utils_Timer_UE::Conv_HandleToTimer(
        const FCk_Handle& InHandle)
        -> FCk_Handle_Timer
{
    CK_ENSURE_IF_NOT(Has(InHandle),
        TEXT("Handle [{}] does NOT have a {}. Unable to convert Handle."),
        InHandle,
        TEXT("Timer")) { return {}; }
    return ck::Cast<FCk_Handle_Timer>(InHandle);
};

auto
    UCk_Utils_Timer_UE::
    Get_Timer(
        const FCk_Handle& InTimerEntity,
        FGameplayTag InTimerName)
    -> FCk_Handle_Timer
{
    const auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerEntity, InTimerName);

    return Conv_HandleToTimer(TimerEntity);
}

auto
    UCk_Utils_Timer_UE::
    Get_Name(
        const FCk_Handle_Timer& InTimerEntity)
    -> FGameplayTag
{
    // TODO: remove the cast once Label is type-safe as well
    return UCk_Utils_GameplayLabel_UE::Get_Label(InTimerEntity.Get_RawHandle());
}

auto
    UCk_Utils_Timer_UE::
    Get_CurrentState(
        const FCk_Handle_Timer& InTimerEntity)
    -> ECk_Timer_State
{
    return InTimerEntity.Has<ck::FTag_Timer_NeedsUpdate>() ? ECk_Timer_State::Running : ECk_Timer_State::Paused;
}

auto
    UCk_Utils_Timer_UE::
    Get_CountDirection(
        const FCk_Handle_Timer& InTimerEntity)
    -> ECk_Timer_CountDirection
{
    return InTimerEntity.Has<ck::FTag_Timer_Countdown>() ?
        ECk_Timer_CountDirection::CountDown :
        ECk_Timer_CountDirection::CountUp;
}

auto
    UCk_Utils_Timer_UE::
    Get_Behavior(
        const FCk_Handle_Timer& InTimerEntity)
    -> ECk_Timer_Behavior
{
    return InTimerEntity.Get<ck::FFragment_Timer_Params>().Get_Params().Get_Behavior();
}

auto
    UCk_Utils_Timer_UE::
    Get_CurrentTimerValue(
        const FCk_Handle_Timer& InTimerEntity)
    -> FCk_Chrono
{
    return InTimerEntity.Get<ck::FFragment_Timer_Current>().Get_Chrono();
}

auto
    UCk_Utils_Timer_UE::
    ForEach_Timer(
        FCk_Handle                 InTimerOwnerEntity,
        const FInstancedStruct&    InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Timer>
{
    auto Timers = TArray<FCk_Handle_Timer>{};

    ForEach_Timer(InTimerOwnerEntity, [&](FCk_Handle_Timer InTimer)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InTimer.Get_RawHandle(), InOptionalPayload); }
        else
        { Timers.Emplace(InTimer); }
    });

    return Timers;
}

auto
    UCk_Utils_Timer_UE::
    ForEach_Timer(
        FCk_Handle InTimerOwnerEntity,
        const TFunction<void(FCk_Handle_Timer)>& InFunc)
    -> void
{
    if (NOT RecordOfTimers_Utils::Has(InTimerOwnerEntity))
    { return; }

    RecordOfTimers_Utils::ForEach_ValidEntry
    (
        InTimerOwnerEntity,
        [&](const FCk_Handle& InTimerEntity)
        {
            InFunc(Conv_HandleToTimer(InTimerEntity));
        }
    );
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    Request_Reset(
        FCk_Handle_Timer& InTimerEntity)
    -> void
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Reset});
}

auto
    UCk_Utils_Timer_UE::
    Request_Complete(
        FCk_Handle_Timer& InTimerEntity)
    -> void
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Complete});
}

auto
    UCk_Utils_Timer_UE::
    Request_Stop(
        FCk_Handle_Timer& InTimerEntity)
    -> void
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Stop});
}

auto
    UCk_Utils_Timer_UE::
    Request_Pause(
        FCk_Handle_Timer& InTimerEntity)
    -> void
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Pause});
}

auto
    UCk_Utils_Timer_UE::
    Request_Resume(
        FCk_Handle_Timer& InTimerEntity)
    -> void
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Resume});
}

auto
    UCk_Utils_Timer_UE::
    Request_Jump(
        FCk_Handle_Timer& InTimerEntity,
        FCk_Request_Timer_Jump InRequest)
    -> void
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Timer_UE::
    Request_Consume(
        FCk_Handle_Timer& InTimerEntity,
        FCk_Request_Timer_Consume InRequest)
    -> void
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Timer_UE::
    Request_ChangeCountDirection(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Timer_CountDirection InCountDirection)
    -> void
{
    switch(InCountDirection)
    {
        case ECk_Timer_CountDirection::CountUp:
        {
            InTimerEntity.Try_Remove<ck::FTag_Timer_Countdown>();
            break;
        }
        case ECk_Timer_CountDirection::CountDown:
        {
            InTimerEntity.AddOrGet<ck::FTag_Timer_Countdown>();
            break;
        }
    }
}

auto
    UCk_Utils_Timer_UE::
    Request_ReverseDirection(
        FCk_Handle_Timer& InTimerEntity)
    -> void
{
    if (InTimerEntity.Has<ck::FTag_Timer_Countdown>())
    { InTimerEntity.Remove<ck::FTag_Timer_Countdown>(); }
    else
    { InTimerEntity.Add<ck::FTag_Timer_Countdown>(); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    BindTo_OnReset(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerReset::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnStop(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerStop::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnPause(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerPause::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnResume(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerResume::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnDone(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerDone::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnDepleted(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerDepleted::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnUpdate(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerUpdate::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnReset(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerReset::Unbind(InTimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnStop(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerStop::Unbind(InTimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnPause(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerPause::Unbind(InTimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnResume(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerResume::Unbind(InTimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnDone(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerDone::Unbind(InTimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnDepleted(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerDepleted::Unbind(InTimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnUpdate(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnTimerUpdate::Unbind(InTimerEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------
