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
        FCk_Handle& InHandle,
        const FCk_Fragment_Timer_ParamsData& InParams)
    -> FCk_Handle_Timer
{
     auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
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

    auto NewTimerEntity = ck::StaticCast<FCk_Handle_Timer>(NewEntity);

    RecordOfTimers_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfTimers_Utils::Request_Connect(InHandle, NewTimerEntity);

    return NewTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    AddOrReplace(
        FCk_Handle& InTimerOwnerEntity,
        const FCk_Fragment_Timer_ParamsData& InParams)
    -> FCk_Handle_Timer
{
    auto MaybeExistingTimerEntity = TryGet_Timer(InTimerOwnerEntity, InParams.Get_TimerName());

    if (ck::Is_NOT_Valid(MaybeExistingTimerEntity))
    { return Add(InTimerOwnerEntity, InParams); }

    MaybeExistingTimerEntity.Replace<ck::FFragment_Timer_Params>(InParams);
    MaybeExistingTimerEntity.Replace<ck::FFragment_Timer_Current>(FCk_Chrono{InParams.Get_Duration()});

    if (InParams.Get_StartingState() == ECk_Timer_State::Running)
    {
        MaybeExistingTimerEntity.AddOrGet<ck::FTag_Timer_NeedsUpdate>();
    }

    return ck::StaticCast<FCk_Handle_Timer>(MaybeExistingTimerEntity);
}

auto
    UCk_Utils_Timer_UE::
    AddMultiple(
        FCk_Handle& InHandle,
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
    UCk_Utils_Timer_UE::
    Has_Any(
        const FCk_Handle& InAttributeOwnerEntity)
    -> bool
{
    return RecordOfTimers_Utils::Has(InAttributeOwnerEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(Timer, UCk_Utils_Timer_UE, FCk_Handle_Timer, ck::FFragment_Timer_Current, ck::FFragment_Timer_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    TryGet_Timer(
        const FCk_Handle& InTimerOwnerEntity,
        FGameplayTag InTimerName)
    -> FCk_Handle_Timer
{
    return RecordOfTimers_Utils::Get_ValidEntry_If(InTimerOwnerEntity,
        ck::algo::MatchesGameplayLabelExact{InTimerName});
}

auto
    UCk_Utils_Timer_UE::
    Get_Name(
        const FCk_Handle_Timer& InTimerEntity)
    -> FGameplayTag
{
    return UCk_Utils_GameplayLabel_UE::Get_Label(InTimerEntity);
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
        FCk_Handle& InTimerOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Timer>
{
    auto Timers = TArray<FCk_Handle_Timer>{};

    ForEach_Timer(InTimerOwnerEntity, [&](FCk_Handle_Timer InTimer)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InTimer, InOptionalPayload); }
        else
        { Timers.Emplace(InTimer); }
    });

    return Timers;
}

auto
    UCk_Utils_Timer_UE::
    ForEach_Timer(
        FCk_Handle& InTimerOwnerEntity,
        const TFunction<void(FCk_Handle_Timer)>& InFunc)
    -> void
{
    RecordOfTimers_Utils::ForEach_ValidEntry(InTimerOwnerEntity, InFunc);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    Request_Reset(
        FCk_Handle_Timer& InTimerEntity)
    -> FCk_Handle_Timer
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Reset});

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    Request_Complete(
        FCk_Handle_Timer& InTimerEntity)
    -> FCk_Handle_Timer
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Complete});

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    Request_Stop(
        FCk_Handle_Timer& InTimerEntity)
    -> FCk_Handle_Timer
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Stop});

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    Request_Pause(
        FCk_Handle_Timer& InTimerEntity)
    -> FCk_Handle_Timer
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Pause});

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    Request_Resume(
        FCk_Handle_Timer& InTimerEntity)
    -> FCk_Handle_Timer
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(
        FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Resume});

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    Request_Jump(
        FCk_Handle_Timer& InTimerEntity,
        FCk_Request_Timer_Jump InRequest)
    -> FCk_Handle_Timer
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(InRequest);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    Request_Consume(
        FCk_Handle_Timer& InTimerEntity,
        FCk_Request_Timer_Consume InRequest)
    -> FCk_Handle_Timer
{
    InTimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(InRequest);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    Request_ChangeCountDirection(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Timer_CountDirection InCountDirection)
    -> FCk_Handle_Timer
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

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    Request_ReverseDirection(
        FCk_Handle_Timer& InTimerEntity)
    -> FCk_Handle_Timer
{
    if (InTimerEntity.Has<ck::FTag_Timer_Countdown>())
    { InTimerEntity.Remove<ck::FTag_Timer_Countdown>(); }
    else
    { InTimerEntity.Add<ck::FTag_Timer_Countdown>(); }

    return InTimerEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    BindTo_OnReset(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerReset::Bind(InTimerEntity, InDelegate, InBindingPolicy);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnStop(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerStop::Bind(InTimerEntity, InDelegate, InBindingPolicy);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnPause(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerPause::Bind(InTimerEntity, InDelegate, InBindingPolicy);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnResume(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerResume::Bind(InTimerEntity, InDelegate, InBindingPolicy);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnDone(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerDone::Bind(InTimerEntity, InDelegate, InBindingPolicy);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnDepleted(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerDepleted::Bind(InTimerEntity, InDelegate, InBindingPolicy);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnUpdate(
        FCk_Handle_Timer& InTimerEntity,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerUpdate::Bind(InTimerEntity, InDelegate, InBindingPolicy);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnReset(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerReset::Unbind(InTimerEntity, InDelegate);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnStop(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerStop::Unbind(InTimerEntity, InDelegate);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnPause(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerPause::Unbind(InTimerEntity, InDelegate);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnResume(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerResume::Unbind(InTimerEntity, InDelegate);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnDone(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerDone::Unbind(InTimerEntity, InDelegate);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnDepleted(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerDepleted::Unbind(InTimerEntity, InDelegate);

    return InTimerEntity;
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnUpdate(
        FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate)
    -> FCk_Handle_Timer
{
    ck::UUtils_Signal_OnTimerUpdate::Unbind(InTimerEntity, InDelegate);

    return InTimerEntity;
}

//w
//--------------------------------------------------------------------------------------------------------------------
