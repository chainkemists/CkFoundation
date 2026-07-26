#include "CkJoltCharacter_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcsExt/PhysicsOwnership/CkPhysicsOwnership_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkJolt/Body/CkJoltBody_Fragment.h"
#include "CkJolt/Character/CkJoltCharacter_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltCharacter_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_JoltCharacter_ParamsData& InParams)
    -> FCk_Handle_JoltCharacter
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle passed to JoltCharacter Add"))
    { return {}; }

    // Writeback + teleport both operate on the entity's own Transform.
    CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
        TEXT("Cannot Add a JoltCharacter to Entity [{}] because it does NOT have the Transform feature."), InHandle)
    { return {}; }

    // Chaos XOR Jolt per entity. A failed claim already ensured inside TryClaim_Jolt.
    if (NOT ck::physics_ownership::TryClaim_Jolt(InHandle))
    { return {}; }

    InHandle.Add<ck::FFragment_JoltCharacter_Params>(InParams);
    InHandle.Add<ck::FFragment_JoltCharacter_Current>();
    // A character interpolates through the JoltBody StepPose + WritebackInterpolated path, hence the reuse.
    InHandle.Add<ck::FFragment_JoltBody_StepPose>();
    InHandle.Add<ck::FTag_JoltCharacter_NeedsSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_JoltCharacter_UE, FCk_Handle_JoltCharacter, ck::FFragment_JoltCharacter_Current, ck::FFragment_JoltCharacter_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltCharacter_UE::
    Get_GroundState(
        const FCk_Handle_JoltCharacter& InJoltCharacter)
    -> ECk_JoltCharacter_GroundState
{
    return InJoltCharacter.Get<ck::FFragment_JoltCharacter_Current>().Get_GroundStateMirror();
}

auto
    UCk_Utils_JoltCharacter_UE::
    Get_GroundNormal(
        const FCk_Handle_JoltCharacter& InJoltCharacter)
    -> FVector
{
    return InJoltCharacter.Get<ck::FFragment_JoltCharacter_Current>().Get_GroundNormalMirror();
}

auto
    UCk_Utils_JoltCharacter_UE::
    Get_GroundVelocity(
        const FCk_Handle_JoltCharacter& InJoltCharacter)
    -> FVector
{
    return InJoltCharacter.Get<ck::FFragment_JoltCharacter_Current>().Get_GroundVelocityMirror();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltCharacter_UE::
    Request_Move(
        FCk_Handle_JoltCharacter& InJoltCharacter,
        const FCk_Request_JoltCharacter_Move& InRequest)
    -> FCk_Handle_JoltCharacter
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltCharacter_Requests, InJoltCharacter);

    InJoltCharacter.AddOrGet<ck::FFragment_JoltCharacter_Requests>()._Requests.Emplace(InRequest);

    return InJoltCharacter;
}

auto
    UCk_Utils_JoltCharacter_UE::
    Request_Jump(
        FCk_Handle_JoltCharacter& InJoltCharacter,
        const FCk_Request_JoltCharacter_Jump& InRequest)
    -> FCk_Handle_JoltCharacter
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltCharacter_Requests, InJoltCharacter);

    InJoltCharacter.AddOrGet<ck::FFragment_JoltCharacter_Requests>()._Requests.Emplace(InRequest);

    return InJoltCharacter;
}

auto
    UCk_Utils_JoltCharacter_UE::
    Request_Teleport(
        FCk_Handle_JoltCharacter& InJoltCharacter,
        const FCk_Request_JoltCharacter_Teleport& InRequest)
    -> FCk_Handle_JoltCharacter
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltCharacter_Requests, InJoltCharacter);

    InJoltCharacter.AddOrGet<ck::FFragment_JoltCharacter_Requests>()._Requests.Emplace(InRequest);

    return InJoltCharacter;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltCharacter_UE::
    BindTo_OnJoltCharacterGroundStateChanged(
        FCk_Handle_JoltCharacter& InJoltCharacter,
        const FCk_Delegate_JoltCharacter_OnGroundStateChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_JoltCharacter
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnJoltCharacterGroundStateChanged, InJoltCharacter, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InJoltCharacter;
}

auto
    UCk_Utils_JoltCharacter_UE::
    UnbindFrom_OnJoltCharacterGroundStateChanged(
        FCk_Handle_JoltCharacter& InJoltCharacter,
        const FCk_Delegate_JoltCharacter_OnGroundStateChanged& InDelegate)
    -> FCk_Handle_JoltCharacter
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnJoltCharacterGroundStateChanged, InJoltCharacter, InDelegate);
    return InJoltCharacter;
}

// --------------------------------------------------------------------------------------------------------------------
