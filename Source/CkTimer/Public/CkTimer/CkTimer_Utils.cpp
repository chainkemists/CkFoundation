#include "CkTimer_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"

#include "CkTimer/CkTimer_Fragment.h"
#include "CkTimer/CkTimer_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Timer_ParamsData& InParams,
        ECk_Net_ReplicationType InReplicationType)
    -> void
{
    if (NOT UCk_Utils_Net_UE::Get_IsEntityRoleMatching(InHandle, InReplicationType))
    {
        ck::timer::Verbose
        (
            TEXT("Skipping creation of Timer [{}] because it's Replication Type [{}] does NOT match"),
            InParams.Get_TimerName(),
            InReplicationType
        );

        return;
    }

    auto ParamsToUse = InParams;
    ParamsToUse.Set_ReplicationType(InReplicationType);

    const auto NewTimerEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
    {
        UCk_Utils_GameplayLabel_UE::Add(InNewEntity, ParamsToUse.Get_TimerName());

        InNewEntity.Add<ck::FFragment_Timer_Params>(ParamsToUse);
        InNewEntity.Add<ck::FFragment_Timer_Current>(FCk_Chrono{ParamsToUse.Get_Duration()});

        if (ParamsToUse.Get_CountDirection() == ECk_Timer_CountDirection::CountDown)
		{ InNewEntity.Add<ck::FTag_Timer_Countdown>(); }

        if (ParamsToUse.Get_StartingState() == ECk_Timer_State::Running)
        {
            InNewEntity.Add<ck::FTag_Timer_NeedsUpdate>();
        }
    });

    RecordOfTimers_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfTimers_Utils::Request_Connect(InHandle, NewTimerEntity);
}

auto
    UCk_Utils_Timer_UE::
    AddOrReplace(
        FCk_Handle                           InTimerOwnerEntity,
        const FCk_Fragment_Timer_ParamsData& InParams)
    -> void
{
    if (NOT Has(InTimerOwnerEntity, InParams.Get_TimerName()))
    {
        Add(InTimerOwnerEntity, InParams);
        return;
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
}

auto
    UCk_Utils_Timer_UE::
    AddMultiple(
        FCk_Handle                                   InHandle,
        const FCk_Fragment_MultipleTimer_ParamsData& InParams)
    -> void
{
    for (const auto& Params : InParams.Get_TimerParams())
    {
        Add(InHandle, Params);
    }
}

auto
    UCk_Utils_Timer_UE::
    Remove(
        FCk_Handle   InTimerOwnerEntity,
        FGameplayTag InTimerName)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(TimerEntity, ECk_EntityLifetime_DestructionBehavior::ForceDestroy);
}

auto
    UCk_Utils_Timer_UE::
    Has(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName)
    -> bool
{
    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    return ck::IsValid(TimerEntity);
}

auto
    UCk_Utils_Timer_UE::
    Has_Any(
        FCk_Handle InTimerOwnerEntity)
    -> bool
{
    return RecordOfTimers_Utils::Has(InTimerOwnerEntity);
}

auto
    UCk_Utils_Timer_UE::
    Ensure_Any(
        FCk_Handle InTimerOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InTimerOwnerEntity), TEXT("Handle [{}] does NOT have any Timer [{}]"), InTimerOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_Timer_UE::
    Get_All(
        FCk_Handle InTimerOwnerEntity)
    -> TArray<FGameplayTag>
{
    if (NOT Has_Any(InTimerOwnerEntity))
    { return {}; }

    auto AllTimers = TArray<FGameplayTag>{};

    RecordOfTimers_Utils::ForEach_ValidEntry(InTimerOwnerEntity, [&](FCk_Handle InTimerEntity)
    {
        AllTimers.Emplace(UCk_Utils_GameplayLabel_UE::Get_Label(InTimerEntity));
    });

    return AllTimers;
}

auto
    UCk_Utils_Timer_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FFragment_Timer_Current, ck::FFragment_Timer_Params>();
}

auto
    UCk_Utils_Timer_UE::
    Ensure(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InTimerOwnerEntity, InTimerName), TEXT("Handle [{}] does NOT have Timer [{}]"), InTimerOwnerEntity, InTimerName)
    { return false; }

    return true;
}

auto
    UCk_Utils_Timer_UE::
    Get_CurrentState(
        FCk_Handle   InTimerOwnerEntity,
        FGameplayTag InTimerName)
    -> ECk_Timer_State
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return {}; }

    const auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    return TimerEntity.Has<ck::FTag_Timer_NeedsUpdate>() ? ECk_Timer_State::Running : ECk_Timer_State::Paused;
}

auto
	UCk_Utils_Timer_UE::
	Get_CountDirection(
		FCk_Handle   InTimerOwnerEntity,
		FGameplayTag InTimerName)
	-> ECk_Timer_CountDirection
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return {}; }

    const auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    if (TimerEntity.Has<ck::FTag_Timer_Countdown>())
    { return ECk_Timer_CountDirection::CountDown; }

	return ECk_Timer_CountDirection::CountUp;
}

auto
    UCk_Utils_Timer_UE::
    Get_Behavior(
        FCk_Handle   InTimerOwnerEntity,
        FGameplayTag InTimerName)
    -> ECk_Timer_Behavior
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return {}; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    return TimerEntity.Get<ck::FFragment_Timer_Params>().Get_Params().Get_Behavior();
}

auto
    UCk_Utils_Timer_UE::
    Get_CurrentTimerValue(
        FCk_Handle   InTimerOwnerEntity,
        FGameplayTag InTimerName)
    -> FCk_Chrono
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return {}; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    return TimerEntity.Get<ck::FFragment_Timer_Current>().Get_Chrono();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    Request_Reset(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    TimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Reset});
}

auto
    UCk_Utils_Timer_UE::
    Request_Complete(
        FCk_Handle   InTimerOwnerEntity,
        FGameplayTag InTimerName)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    TimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Complete});
}

auto
    UCk_Utils_Timer_UE::
    Request_Stop(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    TimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Stop});
}

auto
    UCk_Utils_Timer_UE::
    Request_Pause(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    TimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Pause});
}

auto
    UCk_Utils_Timer_UE::
    Request_Resume(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    TimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Resume});
}

auto
	UCk_Utils_Timer_UE::
	Request_Jump(
		FCk_Handle             InTimerOwnerEntity,
		FGameplayTag           InTimerName,
		FCk_Request_Timer_Jump InRequest)
	-> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    TimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Timer_UE::
    Request_Consume(
        FCk_Handle                InTimerOwnerEntity,
        FGameplayTag              InTimerName,
        FCk_Request_Timer_Consume InRequest)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    TimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(InRequest);
}

auto
	UCk_Utils_Timer_UE::
	Request_ChangeCountDirection(
		FCk_Handle               InTimerOwnerEntity,
		FGameplayTag             InTimerName,
		ECk_Timer_CountDirection InCountDirection)
	-> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    switch(InCountDirection)
    {
	    case ECk_Timer_CountDirection::CountUp:
		{
            TimerEntity.Try_Remove<ck::FTag_Timer_Countdown>();
		    break;
        }
	    case ECk_Timer_CountDirection::CountDown:
		{
            TimerEntity.AddOrGet<ck::FTag_Timer_Countdown>();
		    break;
        }
    }
}

auto
	UCk_Utils_Timer_UE::
	Request_ReverseDirection(
		FCk_Handle   InTimerOwnerEntity,
		FGameplayTag InTimerName)
	-> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    if (TimerEntity.Has<ck::FTag_Timer_Countdown>())
    { TimerEntity.Remove<ck::FTag_Timer_Countdown>(); }
    else
    { TimerEntity.Add<ck::FTag_Timer_Countdown>(); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerReset(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerReset::Bind(TimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerStop(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerStop::Bind(TimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerPause(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerPause::Bind(TimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerResume(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerResume::Bind(TimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerDone(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerDone::Bind(TimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerDepleted(
        FCk_Handle                InTimerOwnerEntity,
        FGameplayTag              InTimerName,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerDepleted::Bind(TimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerUpdate(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerUpdate::Bind(TimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerReset(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerReset::Unbind(TimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerStop(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerStop::Unbind(TimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerPause(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerPause::Unbind(TimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerResume(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerResume::Unbind(TimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerDone(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerDone::Unbind(TimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerDepleted(
        FCk_Handle                InTimerOwnerEntity,
        FGameplayTag              InTimerName,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerDepleted::Unbind(TimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerUpdate(
        FCk_Handle InTimerOwnerEntity,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InTimerOwnerEntity, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InTimerOwnerEntity, InTimerName);

    ck::UUtils_Signal_OnTimerUpdate::Unbind(TimerEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    Request_OverrideTimer(
        FCk_Handle InHandle,
        const FCk_Chrono& InNewTimer)
    -> void
{
    InHandle.Get<ck::FFragment_Timer_Current>() = ck::FFragment_Timer_Current{InNewTimer};
}

// --------------------------------------------------------------------------------------------------------------------
