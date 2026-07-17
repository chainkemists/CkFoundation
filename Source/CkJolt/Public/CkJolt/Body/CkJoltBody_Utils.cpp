#include "CkJoltBody_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcsExt/PhysicsOwnership/CkPhysicsOwnership_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkJolt/Body/CkJoltBody_Fragment.h"

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

    // Writeback + kinematic push both operate on the entity's Transform, so a JoltBody REQUIRES the
    // Transform feature (it composes onto the target entity directly, like a Probe).
    CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
        TEXT("Cannot Add a JoltBody to Entity [{}] because it does NOT have the Transform feature."), InHandle)
    { return {}; }

    // Cross-engine ownership: a Chaos-simulated entity cannot also be Jolt-simulated. Claim fails loudly
    // at the composing site (its own ensure) and returns an invalid handle here.
    if (NOT ck::physics_ownership::TryClaim_Jolt(InHandle))
    { return {}; }

    InHandle.Add<ck::FFragment_JoltBody_Params>(InParams);
    InHandle.Add<ck::FFragment_JoltBody_Current>();
    InHandle.Add<ck::FFragment_JoltBody_StepPose>();
    InHandle.Add<ck::FTag_JoltBody_NeedsSetup>();

    // A body composed Asleep is batch-added DontActivate, and Jolt never fires OnBodyDeactivated for a
    // never-activated body — mirror the intended initial state onto the tag at composition, or
    // Get_SleepState would report Awake until the body's first activate-then-sleep cycle.
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

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltBody_UE::
    Request_SetSleepState(
        FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_SetSleepState& InRequest)
    -> FCk_Handle_JoltBody
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltBody_Requests, InJoltBody);

    InJoltBody.AddOrGet<ck::FFragment_JoltBody_Requests>()._Requests.Emplace(InRequest);

    return InJoltBody;
}

// --------------------------------------------------------------------------------------------------------------------
