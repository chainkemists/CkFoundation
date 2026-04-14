#include "CkSmTask_SubStateMachine.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_SubStateMachine::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    // Schema-only add here so siblings (e.g. UCk_SmCondition_SubSmFinished) can discover this
    // task by fragment during their own Construct/BeginPlay. The _SubStateMachineHandle payload
    // is populated in BeginPlay once the world context is available to spawn the sub-SM.
    InHandle.AddOrGet<ck::FFragment_SmTask_SubStateMachine>();

    return Super::Construct(InHandle, InSpawnParams);
}

auto
    UCk_SmTask_SubStateMachine::
    BeginPlay()
    -> void
{
    auto ScriptEntity = DoGet_ScriptEntity();
    auto TaskEntity = UCk_Utils_SmTask_UE::CastChecked(ScriptEntity);

    CK_ENSURE_IF_NOT(ck::IsValid(_InitialStateClass),
        TEXT("SubStateMachine task [{}] could not resolve sub-SM class.{}"),
        TaskEntity, ck::Context(this))
    {
        Mark_Result(ECk_SmTaskResult::Failed);
        return;
    }

    auto GameEntity = DoGet_GameEntity();

    _SubSmHandle = UCk_Utils_StateMachine_UE::Add(
        ScriptEntity,
        _InitialStateClass,
        ECk_SmAutoStart::Disabled);

    CK_ENSURE_IF_NOT(ck::IsValid(_SubSmHandle),
        TEXT("Failed to create sub-StateMachine for task [{}]"), TaskEntity)
    {
        Mark_Result(ECk_SmTaskResult::Failed);
        return;
    }

    if (ck::IsValid(GameEntity))
    {
        _SubSmHandle.Add<ck::FFragment_Sm_Context>(GameEntity);
    }

    if (const auto OwningStateMachine = ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(TaskEntity);
        OwningStateMachine.Has<ck::FFragment_Sm_StateOverrides>())
    {
        const auto& ParentStateMachineStateOverrides = OwningStateMachine.Get<ck::FFragment_Sm_StateOverrides>();
        _SubSmHandle.Add<ck::FFragment_Sm_StateOverrides>(ParentStateMachineStateOverrides);
    }

    if (ck::TUtils_Sm_ParentState::Has(ScriptEntity))
    {
        if (auto HostingState = ck::TUtils_Sm_ParentState::Get_StoredEntity(ScriptEntity);
            HostingState.Has<ck::FFragment_SmState_Hierarchy>())
        {
            const auto& HostingHierarchy = HostingState.Get<ck::FFragment_SmState_Hierarchy>().Get_Hierarchy();
            _SubSmHandle.Add<ck::FFragment_Sm_ParentHierarchy>(HostingHierarchy);
        }
    }

    auto& SubSmFragment = ScriptEntity.Get<ck::FFragment_SmTask_SubStateMachine>();
    SubSmFragment._SubStateMachineHandle = _SubSmHandle;

    // Broadcast the promise signal so late subscribers (e.g. sibling conditions whose
    // BeginPlay ran before this task's BeginPlay) can discover the sub-SM handle.
    ck::UUtils_Signal_OnSubSmConstructed::Broadcast(TaskEntity,
        ck::MakePayload(TaskEntity, FCk_Sm_Payload_OnSubSmConstructed{_SubSmHandle}));

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
        _InitialStateClass->GetName());

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

void
    UCk_SmTask_SubStateMachine::
    OnSubSmStopped(
        FCk_Handle_StateMachine InHandle,
        FCk_Sm_Payload_OnStopped InPayload)
{
    Mark_Result(ECk_SmTaskResult::Succeeded);
}

// --------------------------------------------------------------------------------------------------------------------
