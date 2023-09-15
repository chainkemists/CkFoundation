#include "CkTimer_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"
#include "CkTimer/CkTimer_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Timer_ParamsData& InData)
    -> void
{
    auto newTimerEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
    ck::UCk_Utils_OwningEntity::Add(newTimerEntity, InHandle);

    newTimerEntity.Add<ck::FFragment_Timer_Params>(InData);
    newTimerEntity.Add<ck::FFragment_Timer_Current>(FCk_Chrono{InData.Get_Duration()});

    UCk_Utils_Ecs_Net_UE::TryAddReplicatedFragment<UCk_Fragment_Timer_Rep>(newTimerEntity);

    if (InData.Get_StartingState() == ECk_Timer_State::Running)
    {
        newTimerEntity.Add<ck::FTag_Timer_NeedsUpdate>();
    }

    UCk_Utils_GameplayLabel_UE::Add(newTimerEntity, InData.Get_TimerName());

    //TODO: Select NoDuplicate Record policy
    RecordOfTimers_Utils::AddIfMissing(InHandle);
    RecordOfTimers_Utils::Request_Connect(InHandle, newTimerEntity);
}

auto
    UCk_Utils_Timer_UE::
    Has(
        FCk_Handle InHandle,
        FGameplayTag InTimerName)
    -> bool
{
    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    return ck::IsValid(TimerEntity);
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
        FCk_Handle InHandle,
        FGameplayTag InTimerName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle, InTimerName), TEXT("Handle [{}] does NOT have Timer [{}]"), InHandle, InTimerName)
    { return false; }

    return true;
}

auto
    UCk_Utils_Timer_UE::
    Get_CurrentState(
        FCk_Handle   InHandle,
        FGameplayTag InTimerName)
    -> ECk_Timer_State
{
    if (NOT Ensure(InHandle, InTimerName))
    { return {}; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    return InHandle.Has<ck::FTag_Timer_NeedsUpdate>() ? ECk_Timer_State::Running : ECk_Timer_State::Paused;
}

auto
    UCk_Utils_Timer_UE::
    Get_Behavior(
        FCk_Handle   InHandle,
        FGameplayTag InTimerName)
    -> ECk_Timer_Behavior
{
    if (NOT Ensure(InHandle, InTimerName))
    { return {}; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    return TimerEntity.Get<ck::FFragment_Timer_Params>().Get_Params().Get_Behavior();
}

auto
    UCk_Utils_Timer_UE::
    Get_CurrentTimerValue(
        FCk_Handle   InHandle,
        FGameplayTag InTimerName)
    -> FCk_Chrono
{
    if (NOT Ensure(InHandle, InTimerName))
    { return {}; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    return TimerEntity.Get<ck::FFragment_Timer_Current>().Get_Chrono();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    Request_Reset(
        FCk_Handle InHandle,
        FGameplayTag InTimerName)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    TimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Reset});
}

auto
    UCk_Utils_Timer_UE::
    Request_Stop(
        FCk_Handle InHandle,
        FGameplayTag InTimerName)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    TimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Stop});
}

auto
    UCk_Utils_Timer_UE::
    Request_Pause(
        FCk_Handle InHandle,
        FGameplayTag InTimerName)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    TimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Pause});
}

auto
    UCk_Utils_Timer_UE::
    Request_Resume(
        FCk_Handle InHandle,
        FGameplayTag InTimerName)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    auto TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    TimerEntity.AddOrGet<ck::FFragment_Timer_Requests>()._ManipulateRequests.Emplace(FCk_Request_Timer_Manipulate{ECk_Timer_Manipulate::Resume});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerReset(
        FCk_Handle InHandle,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    ck::UUtils_Signal_OnTimerReset::Bind(TimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerStop(
        FCk_Handle InHandle,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    ck::UUtils_Signal_OnTimerStop::Bind(TimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerPause(
        FCk_Handle InHandle,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    ck::UUtils_Signal_OnTimerPause::Bind(TimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerResume(
        FCk_Handle InHandle,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    ck::UUtils_Signal_OnTimerResume::Bind(TimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerDone(
        FCk_Handle InHandle,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    ck::UUtils_Signal_OnTimerDone::Bind(TimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    BindTo_OnTimerUpdate(
        FCk_Handle InHandle,
        FGameplayTag InTimerName,
        ECk_Signal_BindingPolicy  InBindingPolicy,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    ck::UUtils_Signal_OnTimerUpdate::Bind(TimerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerReset(
        FCk_Handle InHandle,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    ck::UUtils_Signal_OnTimerReset::Unbind(TimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerStop(
        FCk_Handle InHandle,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    ck::UUtils_Signal_OnTimerStop::Unbind(TimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerPause(
        FCk_Handle InHandle,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    ck::UUtils_Signal_OnTimerPause::Unbind(TimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerResume(
        FCk_Handle InHandle,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    ck::UUtils_Signal_OnTimerResume::Unbind(TimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerDone(
        FCk_Handle InHandle,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

    ck::UUtils_Signal_OnTimerDone::Unbind(TimerEntity, InDelegate);
}

auto
    UCk_Utils_Timer_UE::
    UnbindFrom_OnTimerUpdate(
        FCk_Handle InHandle,
        FGameplayTag InTimerName,
        const FCk_Delegate_Timer& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InTimerName))
    { return; }

    const auto& TimerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Timer_UE,
        RecordOfTimers_Utils>(InHandle, InTimerName);

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
