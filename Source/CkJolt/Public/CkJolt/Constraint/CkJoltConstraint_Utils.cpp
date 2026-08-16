#include "CkJoltConstraint_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkJolt/Body/CkJoltBody_Utils.h"
#include "CkJolt/CkJolt_Log.h"

#include <Jolt/Physics/Constraints/HingeConstraint.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltConstraint_UE::
    Create(
        FCk_Handle_JoltBody& InBodyA,
        const FCk_Fragment_JoltConstraint_ParamsData& InParams)
    -> FCk_Handle_JoltConstraint
{
    CK_ENSURE_IF_NOT(ck::IsValid(InBodyA),
        TEXT("Invalid Body A Handle passed to JoltConstraint Create"))
    { return {}; }

    const auto& OtherBody = InParams.Get_OtherBody();
    const auto BodyBIsWorldAnchor = ck::Is_NOT_Valid(OtherBody);

    if (NOT BodyBIsWorldAnchor)
    {
        CK_ENSURE_IF_NOT(UCk_Utils_JoltBody_UE::Has(OtherBody),
            TEXT("JoltConstraint Create: OtherBody [{}] has NO JoltBody feature."), OtherBody)
        { return {}; }

        CK_ENSURE_IF_NOT(InBodyA.Get_Entity() != OtherBody.Get_Entity(),
            TEXT("JoltConstraint Create: Body A and OtherBody are the SAME entity [{}] — Jolt forbids self-constraints."),
            InBodyA)
        { return {}; }
    }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InBodyA, [&](FCk_Handle InNewEntity)
    {
#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
        UCk_Utils_Handle_UE::Set_DebugName(InNewEntity,
            *ck::Format_UE(TEXT("JoltConstraint [{}]"), InParams.Get_ConstraintType()));
#endif

        InNewEntity.Add<ck::FFragment_JoltConstraint_Params>(InParams);

        auto& Current = InNewEntity.Add<ck::FFragment_JoltConstraint_Current>();
        Current._BodyA = static_cast<FCk_Handle>(InBodyA);
        Current._BodyB = OtherBody;
        Current._BodyBIsWorldAnchor = BodyBIsWorldAnchor;

        InNewEntity.Add<ck::FTag_JoltConstraint_NeedsSetup>();
    });

    return Cast(NewEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_JoltConstraint_UE, FCk_Handle_JoltConstraint, ck::FFragment_JoltConstraint_Current, ck::FFragment_JoltConstraint_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltConstraint_UE::
    Get_ConstraintType(
        const FCk_Handle_JoltConstraint& InConstraint)
    -> ECk_JoltConstraint_Type
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConstraint),
        TEXT("Invalid JoltConstraint Handle passed to Get_ConstraintType"))
    { return ECk_JoltConstraint_Type::Distance; }

    return InConstraint.Get<ck::FFragment_JoltConstraint_Params>().Get_ConstraintType();
}

auto
    UCk_Utils_JoltConstraint_UE::
    Get_IsConstraintAdded(
        const FCk_Handle_JoltConstraint& InConstraint)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConstraint),
        TEXT("Invalid JoltConstraint Handle passed to Get_IsConstraintAdded"))
    { return false; }

    return InConstraint.Get<ck::FFragment_JoltConstraint_Current>().Get_ConstraintAdded();
}

auto
    UCk_Utils_JoltConstraint_UE::
    Get_BodyA(
        const FCk_Handle_JoltConstraint& InConstraint)
    -> FCk_Handle_JoltBody
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConstraint),
        TEXT("Invalid JoltConstraint Handle passed to Get_BodyA"))
    { return {}; }

    return UCk_Utils_JoltBody_UE::CastChecked(
        InConstraint.Get<ck::FFragment_JoltConstraint_Current>().Get_BodyA());
}

auto
    UCk_Utils_JoltConstraint_UE::
    Get_BodyB(
        const FCk_Handle_JoltConstraint& InConstraint)
    -> FCk_Handle_JoltBody
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConstraint),
        TEXT("Invalid JoltConstraint Handle passed to Get_BodyB"))
    { return {}; }

    // CastChecked answers an empty handle for an invalid one, which is exactly what a world-anchored
    // constraint (and one whose body B has died) should report — no ensure for either.
    return UCk_Utils_JoltBody_UE::CastChecked(
        InConstraint.Get<ck::FFragment_JoltConstraint_Current>().Get_BodyB());
}

auto
    UCk_Utils_JoltConstraint_UE::
    Get_IsBodyBWorldAnchor(
        const FCk_Handle_JoltConstraint& InConstraint)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConstraint),
        TEXT("Invalid JoltConstraint Handle passed to Get_IsBodyBWorldAnchor"))
    { return false; }

    return InConstraint.Get<ck::FFragment_JoltConstraint_Current>().Get_BodyBIsWorldAnchor();
}

auto
    UCk_Utils_JoltConstraint_UE::
    Get_Hinge_CurrentAngleDegrees(
        const FCk_Handle_JoltConstraint& InConstraint)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConstraint),
        TEXT("Invalid JoltConstraint Handle passed to Get_Hinge_CurrentAngleDegrees"))
    { return 0.0f; }

    CK_ENSURE_IF_NOT(Get_ConstraintType(InConstraint) == ECk_JoltConstraint_Type::Hinge,
        TEXT("Get_Hinge_CurrentAngleDegrees on JoltConstraint [{}] of type [{}]"),
        InConstraint, Get_ConstraintType(InConstraint))
    { return 0.0f; }

    const auto& Current = InConstraint.Get<ck::FFragment_JoltConstraint_Current>();
    if (Current._Constraint.GetPtr() == nullptr)
    { return 0.0f; }

    const auto* Hinge = static_cast<const JPH::HingeConstraint*>(Current._Constraint.GetPtr());
    return FMath::RadiansToDegrees(Hinge->GetCurrentAngle());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltConstraint_UE::
    Request_SetEnabled(
        FCk_Handle_JoltConstraint& InConstraint,
        const FCk_Request_JoltConstraint_SetEnabled& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_JoltConstraint
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltConstraint_Requests, InConstraint);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InConstraint.AddOrGet<ck::FFragment_JoltConstraint_Requests>()._Requests.Emplace(InRequest);

    return InConstraint;
}

auto
    UCk_Utils_JoltConstraint_UE::
    Request_Distance_SetRange(
        FCk_Handle_JoltConstraint& InConstraint,
        const FCk_Request_JoltConstraint_Distance_SetRange& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_JoltConstraint
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltConstraint_Requests, InConstraint);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InConstraint.AddOrGet<ck::FFragment_JoltConstraint_Requests>()._Requests.Emplace(InRequest);

    return InConstraint;
}

auto
    UCk_Utils_JoltConstraint_UE::
    Request_Hinge_SetMotor(
        FCk_Handle_JoltConstraint& InConstraint,
        const FCk_Request_JoltConstraint_Hinge_SetMotor& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_JoltConstraint
{
    CK_CALLSTACK_RECORD(ck::FFragment_JoltConstraint_Requests, InConstraint);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InConstraint.AddOrGet<ck::FFragment_JoltConstraint_Requests>()._Requests.Emplace(InRequest);

    return InConstraint;
}

// --------------------------------------------------------------------------------------------------------------------
