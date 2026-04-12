#include "CkSmTask_SubStateMachine.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_SubStateMachine::
    BeginPlay()
    -> void
{
    auto ScriptEntity = DoGet_ScriptEntity();
    auto TaskEntity = UCk_Utils_SmTask_UE::CastChecked(ScriptEntity);

    const auto StateClass = Get_SubStateMachineClass(TaskEntity);

    CK_ENSURE_IF_NOT(ck::IsValid(StateClass),
        TEXT("SubStateMachine task [{}] could not resolve sub-SM class.{}"),
        TaskEntity, ck::Context(this))
    { return; }

    auto GameEntity = DoGet_GameEntity();

    _SubSmHandle = UCk_Utils_StateMachine_UE::Add(
        ScriptEntity,
        StateClass,
        ECk_SmAutoStart::Disabled);

    CK_ENSURE_IF_NOT(ck::IsValid(_SubSmHandle),
        TEXT("Failed to create sub-StateMachine for task [{}]"), TaskEntity)
    { return; }

    if (ck::IsValid(GameEntity))
    {
        _SubSmHandle.Add<ck::FFragment_Sm_Context>(GameEntity);
    }

    auto& SubSmFragment = ScriptEntity.AddOrGet<ck::FFragment_SmTask_SubStateMachine>();
    SubSmFragment._SubStateMachineHandle = _SubSmHandle;

    if (_CompletionBehavior == ECk_SmTask_SubSm_CompletionBehavior::SucceedOnStop)
    {
        auto Delegate = FCk_Delegate_Sm_OnStopped{};
        Delegate.BindDynamic(this, &ThisType::OnSubSmStopped);
        UCk_Utils_StateMachine_UE::BindTo_OnStopped(
            _SubSmHandle,
            Delegate,
            ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
            ECk_Signal_PostFireBehavior::DoNothing);
    }

    UCk_Utils_StateMachine_UE::Request_Start(_SubSmHandle);

    ck::sm::Verbose(TEXT("[SubStateMachine] Started sub-SM with initial state [{}]"),
        StateClass->GetName());

    Super::BeginPlay();
}

auto
    UCk_SmTask_SubStateMachine::
    EndPlay()
    -> void
{
    if (ck::IsValid(_SubSmHandle)
        && _CompletionBehavior == ECk_SmTask_SubSm_CompletionBehavior::SucceedOnStop)
    {
        auto Delegate = FCk_Delegate_Sm_OnStopped{};
        Delegate.BindDynamic(this, &ThisType::OnSubSmStopped);
        UCk_Utils_StateMachine_UE::UnbindFrom_OnStopped(_SubSmHandle, Delegate);
    }

    _SubSmHandle = {};
    Super::EndPlay();
}

// --------------------------------------------------------------------------------------------------------------------

TSubclassOf<UCk_SmState_EntityScript>
    UCk_SmTask_SubStateMachine::
    Get_SubStateMachineClass_Implementation(
        FCk_Handle_SmTask InTaskEntity) const
{
    return _InitialStateClass;
}

void
    UCk_SmTask_SubStateMachine::
    OnSubSmStopped(
        FCk_Handle_StateMachine InHandle,
        FCk_Sm_Payload_OnStopped InPayload)
{
    Mark_Result(ECk_SmTaskResult::Succeeded);
}

// --------------------------------------------------------------------------------------------------------------------
