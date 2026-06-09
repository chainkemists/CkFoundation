#include "CkSmTask_SubStateMachine.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/Task/CkSmTask_Utils.h"

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
    EnterTask(
        FCk_Handle_SmTask InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_InitialStateClass),
        TEXT("SubStateMachine task [{}] could not resolve sub-SM class.{}"),
        InHandle, ck::Context(this))
    {
        Mark_Result(ECk_SmTaskResult::Failed);
        return;
    }

    auto ScriptEntity = DoGet_ScriptEntity();
    auto TypeUnsafeSubSmHandle = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(ScriptEntity);
    auto SubSmParams = FCk_Fragment_StateMachine_ParamsData{_InitialStateClass};
    SubSmParams.Set_AutoStart(ECk_SmAutoStart::Disabled);
    _SubSmHandle = UCk_Utils_StateMachine_UE::Add(TypeUnsafeSubSmHandle, SubSmParams);

    CK_ENSURE_IF_NOT(ck::IsValid(_SubSmHandle),
        TEXT("Failed to create sub-StateMachine for task [{}]"), InHandle)
    {
        Mark_Result(ECk_SmTaskResult::Failed);
        return;
    }

    if (const auto OwningStateMachine = ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InHandle);
        ck::IsValid(OwningStateMachine))
    {
        // Link the sub-SM back to its parent SM. Symmetric with how tasks/states/conditions
        // hold OwningStateMachine; lets consumers ask "is this a sub-SM? who owns it?" without
        // walking lifetime → task → owning-SM.
        ck::TUtils_Sm_OwningStateMachine::AddOrReplace(_SubSmHandle, OwningStateMachine);

        _SubSmHandle.Add<ck::FFragment_Sm_NetIdentity>(
            UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(OwningStateMachine),
            ck::statemachine::ComputeNetContext(OwningStateMachine));

        if (OwningStateMachine.Has<ck::FFragment_Sm_StateOverrides>())
        {
            const auto& ParentStateMachineStateOverrides = OwningStateMachine.Get<ck::FFragment_Sm_StateOverrides>();
            _SubSmHandle.Add<ck::FFragment_Sm_StateOverrides>(ParentStateMachineStateOverrides);
        }
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
    // EnterCondition ran before this task's EnterTask) can discover the sub-SM handle.
    ck::UUtils_Signal_OnSubSmConstructed::Broadcast(InHandle,
        ck::MakePayload(InHandle, FCk_Sm_Payload_OnSubSmConstructed{_SubSmHandle}));

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

    Super::EnterTask(InHandle, InNetContext);
}

auto
    UCk_SmTask_SubStateMachine::
    ExitTask(
        FCk_Handle_SmTask InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    if (ck::IsValid(_SubSmHandle))
    {
        if (_CompletionBehavior == ECk_SmTask_SubSm_CompletionBehavior::SucceedOnStop)
        {
            auto Delegate = FCk_Delegate_Sm_OnStopped{};
            Delegate.BindDynamic(this, &ThisType::OnSubSmStopped);
            UCk_Utils_StateMachine_UE::UnbindFrom_OnStopped(_SubSmHandle, Delegate);
        }

        // Recurse: synchronously exit the sub-SM's active state chain so its tasks/conditions
        // get their DoExitTask/DoExitCondition (delegate unbinds, etc.) fired before this task
        // and the sub-SM's eventual cascading destruction. Without this, sub-SM child entities
        // would only have FProcessor_Sm_EndPlay run on them at end-of-frame.
        ck::sm::VeryVerbose(TEXT("[SM Lifecycle] SubStateMachine ExitTask -> recursing into sub-SM [{}]"),
            _SubSmHandle);
        UCk_Utils_StateMachine_UE::Request_ExitStateMachine(_SubSmHandle);
    }
    else
    {
        ck::sm::VeryVerbose(TEXT("[SM Lifecycle] SubStateMachine ExitTask on [{}] — no sub-SM to recurse into"),
            InHandle);
    }

    _SubSmHandle = {};
    Super::ExitTask(InHandle, InNetContext);
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
