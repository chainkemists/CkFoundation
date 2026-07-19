#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkJolt/Constraint/CkJoltConstraint_Fragment_Data.h"

#include <variant>

#include <CoreMinimal.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/Constraints/TwoBodyConstraint.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_JoltConstraint_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_JoltConstraint_Setup;
    class FProcessor_JoltConstraint_HandleRequests;
    class FProcessor_JoltConstraint_LivenessReap;
    class FProcessor_JoltConstraint_EndPlay;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_JoltConstraint_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_JoltConstraint_Params = FCk_Fragment_JoltConstraint_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // Runtime Jolt-constraint state. _Constraint owns the JPH constraint ref for the entity's lifetime;
    // _BodyA/_BodyB are the constrained body ENTITIES (B invalid = world anchor); _ConstraintAdded tracks
    // physics-system membership so teardown only removes what was added.
    struct CKJOLT_API FFragment_JoltConstraint_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_JoltConstraint_Current);

    public:
        friend class FProcessor_JoltConstraint_Setup;
        friend class FProcessor_JoltConstraint_HandleRequests;
        friend class FProcessor_JoltConstraint_LivenessReap;
        friend class FProcessor_JoltConstraint_EndPlay;
        friend class ::UCk_Utils_JoltConstraint_UE;

    private:
        JPH::Ref<JPH::TwoBodyConstraint> _Constraint;
        FCk_Handle _BodyA;
        FCk_Handle _BodyB;
        // An invalid _BodyB means WORLD anchor only when this is set — it distinguishes "anchored to the
        // world by design" from "body B's entity died" (which the liveness reaper must act on).
        bool _BodyBIsWorldAnchor = false;
        bool _ConstraintAdded = false;

    public:
        CK_PROPERTY_GET(_BodyA);
        CK_PROPERTY_GET(_BodyB);
        CK_PROPERTY_GET(_BodyBIsWorldAnchor);
        CK_PROPERTY_GET(_ConstraintAdded);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKJOLT_API FFragment_JoltConstraint_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_JoltConstraint_Requests);

    public:
        friend class FProcessor_JoltConstraint_HandleRequests;
        friend class ::UCk_Utils_JoltConstraint_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_JoltConstraint_SetEnabled,
            FCk_Request_JoltConstraint_Distance_SetRange,
            FCk_Request_JoltConstraint_Hinge_SetMotor>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_JoltConstraint_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
