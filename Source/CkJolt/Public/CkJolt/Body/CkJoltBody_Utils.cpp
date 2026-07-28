#include "CkJoltBody_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcsExt/PhysicsOwnership/CkPhysicsOwnership_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkJolt/Body/CkJoltBody_Fragment.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Subsystem/CkJolt_Subsystem.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltBody_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_JoltBody_ParamsData& InParams)
    -> FCk_Handle_JoltBody
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle passed to JoltBody Add"))
    { return {}; }

    // Writeback + kinematic push both operate on the entity's own Transform.
    CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
        TEXT("Cannot Add a JoltBody to Entity [{}] because it does NOT have the Transform feature."), InHandle)
    { return {}; }

    // Chaos XOR Jolt per entity. A failed claim already ensured inside TryClaim_Jolt.
    if (NOT ck::physics_ownership::TryClaim_Jolt(InHandle))
    { return {}; }

    InHandle.Add<ck::FFragment_JoltBody_Params>(InParams);
    InHandle.Add<ck::FFragment_JoltBody_Current>();
    InHandle.Add<ck::FFragment_JoltBody_StepPose>();
    InHandle.Add<ck::FTag_JoltBody_NeedsSetup>();

    // Jolt never fires OnBodyDeactivated for a never-activated body, so a body composed Asleep needs the tag
    // stamped here or Get_SleepState reports Awake until its first activate-then-sleep cycle.
    if (InParams.Get_InitialSleepState() == ECk_Jolt_SleepState::Asleep)
    { InHandle.Add<ck::FTag_JoltBody_Sleeping>(); }

    switch (InParams.Get_MotionType())
    {
        case ECk_MotionType::Static:
        {
            InHandle.Add<ck::FTag_JoltBody_MotionType_Static>();
            break;
        }
        case ECk_MotionType::Kinematic:
        {
            InHandle.Add<ck::FTag_JoltBody_MotionType_Kinematic>();
            InHandle.Add<ck::FTag_JoltBody_KinematicFromECS>();
            break;
        }
        case ECk_MotionType::Dynamic:
        {
            InHandle.Add<ck::FTag_JoltBody_MotionType_Dynamic>();
            break;
        }
    }

    if (InParams.Get_PersistContacts() == ECk_EnableDisable::Enable)
    {
        InHandle.Add<ck::FTag_JoltBody_PersistContacts>();
    }

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_JoltBody_UE, FCk_Handle_JoltBody, ck::FFragment_JoltBody_Current, ck::FFragment_JoltBody_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltBody_UE::
    Get_MotionType(
        const FCk_Handle_JoltBody& InJoltBody)
    -> ECk_MotionType
{
    return InJoltBody.Get<ck::FFragment_JoltBody_Params>().Get_MotionType();
}

auto
    UCk_Utils_JoltBody_UE::
    Get_SleepState(
        const FCk_Handle_JoltBody& InJoltBody)
    -> ECk_Jolt_SleepState
{
    return InJoltBody.Has<ck::FTag_JoltBody_Sleeping>()
        ? ECk_Jolt_SleepState::Asleep
        : ECk_Jolt_SleepState::Awake;
}

auto
    UCk_Utils_JoltBody_UE::
    Get_IsBodyAdded(
        const FCk_Handle_JoltBody& InJoltBody)
    -> bool
{
    return InJoltBody.Get<ck::FFragment_JoltBody_Current>().Get_BodyAdded();
}

auto
    UCk_Utils_JoltBody_UE::
    Get_LinearVelocity(
        const FCk_Handle_JoltBody& InJoltBody)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InJoltBody),
        TEXT("Invalid JoltBody Handle passed to Get_LinearVelocity"))
    { return FVector::ZeroVector; }

    // Not-yet-added is a legitimate transient state (setup is deferred) — report zero, never ensure.
    const auto& Current = InJoltBody.Get<ck::FFragment_JoltBody_Current>();
    if (NOT Current.Get_BodyAdded())
    { return FVector::ZeroVector; }

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InJoltBody);
    if (ck::Is_NOT_Valid(World))
    { return FVector::ZeroVector; }

    const auto JoltSubsystem = World->GetSubsystem<UCk_Jolt_Subsystem>();
    if (ck::Is_NOT_Valid(JoltSubsystem, ck::IsValid_Policy_NullptrOnly{}))
    { return FVector::ZeroVector; }

    const auto PhysicsSystem = JoltSubsystem->Get_PhysicsSystem().Pin();
    if (ck::Is_NOT_Valid(PhysicsSystem))
    { return FVector::ZeroVector; }

    return ck::jolt::Conv(PhysicsSystem->GetBodyInterface().GetLinearVelocity(Current.Get_BodyId()));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltBody_UE::
    Request_SetSleepState(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_SetSleepState& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltBody_Requests, InJoltBody);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InJoltBody.AddOrGet<ck::FFragment_JoltBody_Requests>()._Requests.Emplace(InRequest);

    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    Request_AddForce(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_AddForce& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltBody_Requests, InJoltBody);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InJoltBody.AddOrGet<ck::FFragment_JoltBody_Requests>()._Requests.Emplace(InRequest);

    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    Request_AddForceAtLocation(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_AddForceAtLocation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltBody_Requests, InJoltBody);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InJoltBody.AddOrGet<ck::FFragment_JoltBody_Requests>()._Requests.Emplace(InRequest);

    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    Request_AddTorque(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_AddTorque& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltBody_Requests, InJoltBody);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InJoltBody.AddOrGet<ck::FFragment_JoltBody_Requests>()._Requests.Emplace(InRequest);

    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    Request_AddImpulse(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_AddImpulse& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltBody_Requests, InJoltBody);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InJoltBody.AddOrGet<ck::FFragment_JoltBody_Requests>()._Requests.Emplace(InRequest);

    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    Request_AddImpulseAtLocation(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_AddImpulseAtLocation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltBody_Requests, InJoltBody);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InJoltBody.AddOrGet<ck::FFragment_JoltBody_Requests>()._Requests.Emplace(InRequest);

    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    Request_AddAngularImpulse(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_AddAngularImpulse& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltBody_Requests, InJoltBody);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InJoltBody.AddOrGet<ck::FFragment_JoltBody_Requests>()._Requests.Emplace(InRequest);

    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    Request_SetLinearVelocity(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_SetLinearVelocity& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltBody_Requests, InJoltBody);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InJoltBody.AddOrGet<ck::FFragment_JoltBody_Requests>()._Requests.Emplace(InRequest);

    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    Request_SetAngularVelocity(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_SetAngularVelocity& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltBody_Requests, InJoltBody);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InJoltBody.AddOrGet<ck::FFragment_JoltBody_Requests>()._Requests.Emplace(InRequest);

    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    Request_Teleport(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_Teleport& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltBody_Requests, InJoltBody);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InJoltBody.AddOrGet<ck::FFragment_JoltBody_Requests>()._Requests.Emplace(InRequest);

    return InJoltBody;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltBody_UE::
    BindTo_OnJoltBodyContactAdded(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnContact& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_JoltBody
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnJoltBodyContactAdded, InJoltBody, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    UnbindFrom_OnJoltBodyContactAdded(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnContact& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnJoltBodyContactAdded, InJoltBody, InDelegate);
    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    BindTo_OnJoltBodyContactPersisted(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnContact& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_JoltBody
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnJoltBodyContactPersisted, InJoltBody, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    UnbindFrom_OnJoltBodyContactPersisted(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnContact& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnJoltBodyContactPersisted, InJoltBody, InDelegate);
    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    BindTo_OnJoltBodyContactRemoved(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnContactRemoved& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_JoltBody
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnJoltBodyContactRemoved, InJoltBody, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    UnbindFrom_OnJoltBodyContactRemoved(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnContactRemoved& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnJoltBodyContactRemoved, InJoltBody, InDelegate);
    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    BindTo_OnJoltBodySleepStateChanged(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnSleepStateChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_JoltBody
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnJoltBodySleepStateChanged, InJoltBody, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InJoltBody;
}

auto
    UCk_Utils_JoltBody_UE::
    UnbindFrom_OnJoltBodySleepStateChanged(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnSleepStateChanged& InDelegate)
    -> FCk_Handle_JoltBody
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnJoltBodySleepStateChanged, InJoltBody, InDelegate);
    return InJoltBody;
}

// --------------------------------------------------------------------------------------------------------------------
