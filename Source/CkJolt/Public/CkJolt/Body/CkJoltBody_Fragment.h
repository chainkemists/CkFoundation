#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkJolt/Body/CkJoltBody_Fragment_Data.h"

#include <variant>

#include <CoreMinimal.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_JoltBody_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_JoltBody_Setup;
    class FProcessor_JoltBody_HandleRequests;
    class FProcessor_JoltBody_SleepStateMirror;
    class FProcessor_JoltBody_KinematicPush;
    class FProcessor_JoltBody_WritebackInterpolated;
    class FProcessor_JoltBody_EndPlay;

    // --------------------------------------------------------------------------------------------------------------------

    // Set by the Jolt step's pose-apply pass (FJoltWorld::DoApplyPoseBuffer_GameThread) whenever an
    // active body's StepPose was refreshed this frame. FProcessor_JoltBody_WritebackInterpolated clears
    // it after pushing the interpolated pose onto the entity's Transform.
    CK_DEFINE_ECS_TAG(FTag_JoltBody_TransformDirty);

    CK_DEFINE_ECS_TAG(FTag_JoltBody_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_JoltBody_MotionType_Static);
    CK_DEFINE_ECS_TAG(FTag_JoltBody_MotionType_Kinematic);
    CK_DEFINE_ECS_TAG(FTag_JoltBody_MotionType_Dynamic);
    CK_DEFINE_ECS_TAG(FTag_JoltBody_KinematicFromECS);
    CK_DEFINE_ECS_TAG(FTag_JoltBody_Sleeping);
    CK_DEFINE_ECS_TAG(FTag_JoltBody_PersistContacts);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_JoltBody_Params = FCk_Fragment_JoltBody_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // Latest + previous simulated pose (UE-space) for an active Jolt body, captured post-step off the
    // pose buffer. Prev/Curr let FProcessor_JoltBody_WritebackInterpolated blend by the step alpha.
    struct CKJOLT_API FFragment_JoltBody_StepPose
    {
    public:
        CK_GENERATED_BODY(FFragment_JoltBody_StepPose);

    private:
        FVector _PrevLocation = FVector::ZeroVector;
        FQuat   _PrevRotation = FQuat::Identity;
        FVector _CurrLocation = FVector::ZeroVector;
        FQuat   _CurrRotation = FQuat::Identity;

    public:
        CK_PROPERTY(_PrevLocation);
        CK_PROPERTY(_PrevRotation);
        CK_PROPERTY(_CurrLocation);
        CK_PROPERTY(_CurrRotation);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_JoltBody_StepPose, _PrevLocation, _PrevRotation, _CurrLocation, _CurrRotation);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Runtime Jolt-body handle state. _BodyId is the raw JPH::BodyID (mirroring FFragment_Probe_Current);
    // _Shape retains the shape ref for the body's lifetime; _BodyAdded tracks physics-system membership.
    struct CKJOLT_API FFragment_JoltBody_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_JoltBody_Current);

    public:
        friend class FProcessor_JoltBody_Setup;
        friend class FProcessor_JoltBody_HandleRequests;
        friend class FProcessor_JoltBody_SleepStateMirror;
        friend class FProcessor_JoltBody_KinematicPush;
        friend class FProcessor_JoltBody_WritebackInterpolated;
        friend class FProcessor_JoltBody_EndPlay;
        friend class ::UCk_Utils_JoltBody_UE;

    private:
        JPH::BodyID          _BodyId;
        JPH::Ref<JPH::Shape> _Shape;
        bool                 _BodyAdded = false;

    public:
        CK_PROPERTY_GET(_BodyId);
        CK_PROPERTY_GET(_BodyAdded);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKJOLT_API FFragment_JoltBody_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_JoltBody_Requests);

    public:
        friend class FProcessor_JoltBody_HandleRequests;
        friend class ::UCk_Utils_JoltBody_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_JoltBody_SetSleepState,
            FCk_Request_JoltBody_AddForce,
            FCk_Request_JoltBody_AddForceAtLocation,
            FCk_Request_JoltBody_AddTorque,
            FCk_Request_JoltBody_AddImpulse,
            FCk_Request_JoltBody_AddImpulseAtLocation,
            FCk_Request_JoltBody_AddAngularImpulse,
            FCk_Request_JoltBody_SetLinearVelocity,
            FCk_Request_JoltBody_SetAngularVelocity,
            FCk_Request_JoltBody_Teleport>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_JoltBody_Requests);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKJOLT_API,
        OnJoltBodyContactAdded,
        FCk_Delegate_JoltBody_OnContact,
        FCk_Handle_JoltBody,
        FCk_JoltBody_Payload_OnContact);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKJOLT_API,
        OnJoltBodyContactPersisted,
        FCk_Delegate_JoltBody_OnContact,
        FCk_Handle_JoltBody,
        FCk_JoltBody_Payload_OnContact);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKJOLT_API,
        OnJoltBodyContactRemoved,
        FCk_Delegate_JoltBody_OnContactRemoved,
        FCk_Handle_JoltBody,
        FCk_JoltBody_Payload_OnContactRemoved);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKJOLT_API,
        OnJoltBodySleepStateChanged,
        FCk_Delegate_JoltBody_OnSleepStateChanged,
        FCk_Handle_JoltBody,
        ECk_Jolt_SleepState);
}

// --------------------------------------------------------------------------------------------------------------------
