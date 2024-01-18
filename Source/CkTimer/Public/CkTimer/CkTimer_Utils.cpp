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
    -> FCk_Handle
{
    const auto NewTimerEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
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
    });

    RecordOfTimers_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfTimers_Utils::Request_Connect(InHandle, NewTimerEntity);

    return NewTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    AddOrReplace(
        FCk_Handle InTimerOwnerEntity,
        const FCk_Fragment_Timer_ParamsData& InParams)
    -> FCk_Handle
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

    return TimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleTimer_ParamsData& InParams)
    -> TArray<FCk_Handle>
{
    return ck::algo::Transform<TArray<FCk_Handle>>(InParams.Get_TimerParams(),
    [&](const FCk_Fragment_Timer_ParamsData& InTimerParams)
    {
        return Add(InHandle, InTimerParams);
    });
}

auto
    UCk_Utils_Timer_UE::
    Has(
        FCk_Handle InTimerEntity)
    -> bool
{
    return InTimerEntity.Has_All<ck::FFragment_Timer_Params, ck::FFragment_Timer_Current>();
}

auto
    UCk_Utils_Timer_UE::
    Ensure(
        FCk_Handle InTimerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InTimerEntity), TEXT("Handle [{}] does NOT have a Timer"), InTimerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_Timer_UE::
    Get_Timer(
        FCk_Handle   InTimerEntity,
        FGameplayTag InTimerName)
    -> FCk_Handle
{
    const auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerEntity, InTimerName);

    return TimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    Get_Name(
        FCk_Handle InTimerEntity)
    -> FGameplayTag
{
    if (NOT Ensure(InTimerEntity))
    { return {}; }

    return UCk_Utils_GameplayLabel_UE::Get_Label(InTimerEntity);
}

auto
    UCk_Utils_Timer_UE::
    Get_CurrentState(
        FCk_Handle   InTimerEntity)
    -> ECk_Timer_State
{
    if (NOT Ensure(InTimerEntity))
    { return {}; }

    return InTimerEntity.Has<ck::FTag_Timer_NeedsUpdate>() ? ECk_Timer_State::Running : ECk_Timer_State::Paused;
}

auto
    UCk_Utils_Timer_UE::
    Get_CountDirection(
        FCk_Handle   InTimerEntity)
    -> ECk_Timer_CountDirection
{
    if (NOT Ensure(InTimerEntity))
    { return {}; }

    return InTimerEntity.Has<ck::FTag_Timer_Countdown>() ?
        ECk_Timer_CountDirection::CountDown :
        ECk_Timer_CountDirection::CountUp;
}

auto
    UCk_Utils_Timer_UE::
    Get_Behavior(
        FCk_Handle   InTimerEntity)
    -> ECk_Timer_Behavior
{
    if (NOT Ensure(InTimerEntity))
    { return {}; }

    return InTimerEntity.Get<ck::FFragment_Timer_Params>().Get_Params().Get_Behavior();
}

auto
    UCk_Utils_Timer_UE::
    Get_CurrentTimerValue(
        FCk_Handle   InTimerEntity)
    -> FCk_Chrono
{
    if (NOT Ensure(InTimerEntity))
    { return {}; }

    return InTimerEntity.Get<ck::FFragment_Timer_Current>().Get_Chrono();
}

auto
    UCk_Utils_Timer_UE::
    ForEach_Timer(
        FCk_Handle                 InTimerOwnerEntity,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle>
{
    auto Timers = TArray<FCk_Handle>{};

    ForEach_Timer(InTimerOwnerEntity, [&](FCk_Handle InTimer)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InTimer); }
        else
        { Timers.Emplace(InTimer); }
    });

    return Timers;
}

auto
    UCk_Utils_Timer_UE::
    ForEach_Timer(
        FCk_Handle InTimerOwnerEntity,
        const TFunction<void(FCk_Handle)>& InFunc)
    -> void
{
    if (NOT RecordOfTimers_Utils::Has(InTimerOwnerEntity))
    { return; }

    RecordOfTimers_Utils::ForEach_ValidEntry
    (
        InTimerOwnerEntity,
        [&](const FCk_Handle& InTimerEntity)
        {
            InFunc(InTimerEntity);
        }
    );
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    Request_Reset(
        FCk_Handle InTimerEntity)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }


    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Reset});
}

auto
    UCk_Utils_Timer_UE::
    Request_Complete(
        FCk_Handle   InTimerEntity)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Complete});
}

auto
    UCk_Utils_Timer_UE::
    Request_Stop(
        FCk_Handle InTimerEntity)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Stop});
}

auto
    UCk_Utils_Timer_UE::
    Request_Pause(
        FCk_Handle InTimerEntity)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }


    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Pause});
}

auto
    UCk_Utils_Timer_UE::
    Request_Resume(
        FCk_Handle InTimerEntity)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }


    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Resume});
}

auto
    UCk_Utils_Timer_UE::
    Request_Jump(
        FCk_Handle             InTimerEntity,
        FCk_Request_Timer_Jump InRequest)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Timer_UE::
    Request_Consume(
        FCk_Handle                InTimerEntity,
        FCk_Request_Timer_Consume InRequest)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Timer_UE::
    Request_ChangeCountDirection(
        FCk_Handle               InTimerEntity,
        ECk_Timer_CountDirection InCountDirection)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

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
        FCk_Handle   InTimerEntity)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    if (InTimerEntity.Has<ck::FTag_Timer_Countdown>())
    { InTimerEntity.Remove<ck::FTag_Timer_Countdown>(); }
    else
    { InTimerEntity.Add<ck::FTag_Timer_Countdown>(); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerReset(
        FCk_Handle InTimerEntity,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerReset::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerStop(
        FCk_Handle InTimerEntity,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerStop::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerPause(
        FCk_Handle InTimerEntity,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerPause::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerResume(
        FCk_Handle InTimerEntity,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerResume::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerDone(
        FCk_Handle InTimerEntity,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerDone::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerDepleted(
        FCk_Handle                InTimerEntity,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerDepleted::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerUpdate(
        FCk_Handle InTimerEntity,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerUpdate::Bind(InTimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerReset(
        FCk_Handle InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerReset::Unbind(InTimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerStop(
        FCk_Handle InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerStop::Unbind(InTimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerPause(
        FCk_Handle InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerPause::Unbind(InTimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerResume(
        FCk_Handle InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerResume::Unbind(InTimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerDone(
        FCk_Handle InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerDone::Unbind(InTimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerDepleted(
        FCk_Handle                InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerDepleted::Unbind(InTimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerUpdate(
        FCk_Handle InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerEntity))
    { return; }

    ck::UUtils_Signal_OnTimerUpdate::Unbind(InTimerEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------
