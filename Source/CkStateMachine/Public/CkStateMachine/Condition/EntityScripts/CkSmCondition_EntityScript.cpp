#include "CkSmCondition_EntityScript.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/Condition/CkSmCondition_Utils.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_GraphWalk_Fragment.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    return Super::Construct(InHandle, InSpawnParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_EntityScript::
    Get_EffectiveReplication() const
    -> ECk_Replication
{
    if (ck::IsValid(_AssociatedEntity) && ck::TUtils_Sm_OwningStateMachine::Has(_AssociatedEntity))
    {
        const auto SmHandle = ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(_AssociatedEntity);
        if (ck::IsValid(SmHandle) && SmHandle.Has<ck::FFragment_Sm_Params>())
        {
            return SmHandle.Get<ck::FFragment_Sm_Params>().Get_Replication();
        }
    }
    return Super::Get_EffectiveReplication();
}

auto
    UCk_SmCondition_EntityScript::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    // See UCk_SmState_EntityScript::BeginPlay for rationale. Graph-walk temp conditions
    // must not activate — their DoEnterCondition can have observable side effects
    // (e.g. capturing world time for a dwell timer) and FTag_SmCondition_Active would
    // admit them to the polled evaluation loop.
    if (_AssociatedEntity.Has<ck::FTag_Sm_Debug_GraphWalkEntity>())
    { return; }

    auto Self = UCk_Utils_SmCondition_UE::CastChecked(_AssociatedEntity);
    const auto SmHandle = UCk_Utils_SmCondition_UE::Get_OwningStateMachine(Self);
    const auto NetContext = ck::statemachine::ComputeNetContext(SmHandle);
    EnterCondition(Self, NetContext);
}

auto
    UCk_SmCondition_EntityScript::
    EndPlay()
    -> void
{
    // Fallback for cascade-destroyed conditions whose FProcessor_SmCondition_Exit never runs
    // (e.g. conditions in a sub-SM destroyed via parent state cascade). Dedup'd via
    // FTag_SmCondition_Active inside ExitCondition — if the processor already handled this
    // condition, the tag is gone and ExitCondition is a no-op.
    auto Self = UCk_Utils_SmCondition_UE::CastChecked(_AssociatedEntity);
    if (ck::IsValid(Self))
    {
        const auto SmHandle = UCk_Utils_SmCondition_UE::Get_OwningStateMachine(Self);
        const auto NetContext = ck::statemachine::ComputeNetContext(SmHandle);
        ExitCondition(Self, NetContext);
    }
    Super::EndPlay();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_EntityScript::
    EnterCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    ck::sm::VeryVerbose(TEXT("[SM Lifecycle] EnterCondition [{}] on entity [{}]"), GetClass(), InHandle);

    InHandle.AddOrGet<ck::FTag_SmCondition_Active>();
    DoEnterCondition(InHandle, InNetContext);
}

auto
    UCk_SmCondition_EntityScript::
    ExitCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    if (NOT InHandle.Has<ck::FTag_SmCondition_Active>())
    { return; }

    ck::sm::VeryVerbose(TEXT("[SM Lifecycle] ExitCondition [{}] on entity [{}]"), GetClass(), InHandle);

    InHandle.Try_Remove<ck::FTag_SmCondition_Active>();
    DoExitCondition(InHandle, InNetContext);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle_StateMachine
    UCk_SmCondition_EntityScript::
    Get_OwningStateMachine() const
{
    auto ConditionHandle = DoGet_ScriptEntity();

    if (ck::Is_NOT_Valid(ConditionHandle))
    { return {}; }

    if (NOT ck::TUtils_Sm_OwningStateMachine::Has(ConditionHandle))
    { return {}; }

    return ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(ConditionHandle);
}

FCk_Handle_SmTransition
    UCk_SmCondition_EntityScript::
    Get_ParentTransition() const
{
    auto ConditionHandle = DoGet_ScriptEntity();

    if (ck::Is_NOT_Valid(ConditionHandle))
    { return {}; }

    if (NOT ck::TUtils_Sm_ParentTransition::Has(ConditionHandle))
    { return {}; }

    return ck::TUtils_Sm_ParentTransition::Get_StoredEntity(ConditionHandle);
}

auto
    UCk_SmCondition_EntityScript::
    Get_StateMachineContext() const
    -> FCk_Handle
{
    return UCk_Utils_ContextOwner_UE::Get_ContextOwner(DoGet_ScriptEntity());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_EntityScript::
    Get_ConditionTag() const
    -> FGameplayTag
{
    return UCk_Utils_Object_UE::Get_TagFromClassName(
        GetClass(),
        ck::Format_UE(TEXT("Auto-generated condition tag for {}"), GetClass()));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_EntityScript::
    Get_ConditionTagForClass(
        TSubclassOf<UCk_SmCondition_EntityScript> InClass)
    -> FGameplayTag
{
    CK_ENSURE_IF_NOT(ck::IsValid(InClass),
        TEXT("Invalid condition class in Get_ConditionTagForClass"))
    { return {}; }

    return UCk_Utils_Object_UE::Get_TagFromClassName(
        InClass,
        ck::Format_UE(TEXT("Auto-generated condition tag for {}"), *InClass->GetName()));
}

// --------------------------------------------------------------------------------------------------------------------
