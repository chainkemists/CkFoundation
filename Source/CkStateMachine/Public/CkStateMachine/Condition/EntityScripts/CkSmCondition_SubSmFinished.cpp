#include "CkSmCondition_SubSmFinished.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/Task/CkSmTask_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_SubSmFinished::
    BeginPlay()
    -> void
{
    auto ConditionHandle = DoGet_ScriptEntity();

    const auto ParentTransition = Get_ParentTransition();
    if (ck::Is_NOT_Valid(ParentTransition))
    {
        Super::BeginPlay();
        return;
    }

    if (NOT ck::TUtils_Sm_ParentState::Has(ParentTransition))
    {
        Super::BeginPlay();
        return;
    }

    const auto ParentState = ck::TUtils_Sm_ParentState::Get_StoredEntity(ParentTransition);

    UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::ForEach_ValidEntry(ParentState,
    [&](FCk_Handle_SmTask InTask)
    {
        if (NOT InTask.Has<ck::FFragment_SmTask_SubStateMachine>())
        { return; }

        const auto SubSm = InTask.Get<ck::FFragment_SmTask_SubStateMachine>()
            .Get_SubStateMachineHandle();

        if (ck::IsValid(SubSm))
        {
            DoBindToSubSm(SubSm);
            return;
        }

        // Sub-SM handle not yet populated (task BeginPlay ordering race).
        // Bind a promise to the task's OnSubSmConstructed signal — it will fire
        // as soon as the sub-SM is created (or replay immediately if already fired).
        auto TaskHandle = InTask;
        auto PromiseDelegate = FCk_Delegate_SmTask_OnSubSmConstructed{};
        PromiseDelegate.BindDynamic(this, &ThisType::OnSubSmConstructed);
        UCk_Utils_SmTask_UE::BindTo_OnSubSmConstructed(
            TaskHandle,
            PromiseDelegate,
            ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
            ECk_Signal_PostFireBehavior::Unbind);
        _PendingTasks.Add(InTask);
    });

    Super::BeginPlay();
}

auto
    UCk_SmCondition_SubSmFinished::
    EndPlay()
    -> void
{
    for (auto& PendingTask : _PendingTasks)
    {
        if (ck::Is_NOT_Valid(PendingTask))
        { continue; }

        auto PromiseDelegate = FCk_Delegate_SmTask_OnSubSmConstructed{};
        PromiseDelegate.BindDynamic(this, &ThisType::OnSubSmConstructed);
        UCk_Utils_SmTask_UE::UnbindFrom_OnSubSmConstructed(PendingTask, PromiseDelegate);
    }
    _PendingTasks.Reset();

    for (auto& BoundSubSm : _BoundSubSms)
    {
        if (ck::Is_NOT_Valid(BoundSubSm))
        { continue; }

        auto Delegate = FCk_Delegate_Sm_OnStopped{};
        Delegate.BindDynamic(this, &ThisType::OnSubSmStopped);
        UCk_Utils_StateMachine_UE::UnbindFrom_OnStopped(BoundSubSm, Delegate);
    }
    _BoundSubSms.Reset();

    Super::EndPlay();
}

auto
    UCk_SmCondition_SubSmFinished::
    DoBindToSubSm(
        FCk_Handle_StateMachine InSubSm)
    -> void
{
    auto SubSmHandle = InSubSm;
    auto Delegate = FCk_Delegate_Sm_OnStopped{};
    Delegate.BindDynamic(this, &ThisType::OnSubSmStopped);
    UCk_Utils_StateMachine_UE::BindTo_OnStopped(
        SubSmHandle,
        Delegate,
        ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior::DoNothing);
    _BoundSubSms.Add(InSubSm);
}

void
    UCk_SmCondition_SubSmFinished::
    OnSubSmConstructed(
        FCk_Handle_SmTask InTaskHandle,
        FCk_Sm_Payload_OnSubSmConstructed InPayload)
{
    const auto SubSm = InPayload.Get_SubStateMachineHandle();
    if (ck::Is_NOT_Valid(SubSm))
    { return; }

    DoBindToSubSm(SubSm);
}

void
    UCk_SmCondition_SubSmFinished::
    OnSubSmStopped(
        FCk_Handle_StateMachine InHandle,
        FCk_Sm_Payload_OnStopped InPayload)
{
    MarkSatisfied();
}

// --------------------------------------------------------------------------------------------------------------------
