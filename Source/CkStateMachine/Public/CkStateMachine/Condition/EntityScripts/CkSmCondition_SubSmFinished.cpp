#include "CkSmCondition_SubSmFinished.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

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

    auto ParentStateHandle = static_cast<FCk_Handle>(ParentState);

    UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::ForEach_ValidEntry(ParentStateHandle,
        [&](FCk_Handle_SmTask InTask)
        {
            auto TaskHandle = static_cast<FCk_Handle>(InTask);

            if (NOT TaskHandle.Has<ck::FFragment_SmTask_SubStateMachine>())
            { return; }

            const auto SubSm = TaskHandle.Get<ck::FFragment_SmTask_SubStateMachine>()
                .Get_SubStateMachineHandle();

            if (ck::Is_NOT_Valid(SubSm))
            { return; }

            auto SubSmHandle = SubSm;
            auto Delegate = FCk_Delegate_Sm_OnStopped{};
            Delegate.BindDynamic(this, &ThisType::OnSubSmStopped);
            UCk_Utils_StateMachine_UE::BindTo_OnStopped(
                SubSmHandle,
                Delegate,
                ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
                ECk_Signal_PostFireBehavior::DoNothing);
            _BoundSubSms.Add(SubSm);
        });

    Super::BeginPlay();
}

auto
    UCk_SmCondition_SubSmFinished::
    EndPlay()
    -> void
{
    for (auto& BoundSubSm : _BoundSubSms)
    {
        if (NOT ck::IsValid(BoundSubSm))
        { continue; }

        auto Delegate = FCk_Delegate_Sm_OnStopped{};
        Delegate.BindDynamic(this, &ThisType::OnSubSmStopped);
        UCk_Utils_StateMachine_UE::UnbindFrom_OnStopped(BoundSubSm, Delegate);
    }
    _BoundSubSms.Reset();

    Super::EndPlay();
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
