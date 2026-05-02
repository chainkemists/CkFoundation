#include "CkCrowdAgent_Utils.h"

#include "CkCrowd/CkCrowd_Log.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Add(
        FCk_Handle& InOwner,
        const FCk_Fragment_CrowdAgent_ParamsData& InParams)
    -> FCk_Handle_CrowdAgent
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
        TEXT("Invalid owner handle [{}] passed to UCk_Utils_CrowdAgent_UE::Add"), InOwner)
    { return {}; }

    auto NewAgentEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_CrowdAgent>(InOwner);

    NewAgentEntity.Add<ck::FFragment_CrowdAgent_Params>(InParams);
    NewAgentEntity.Add<ck::FFragment_CrowdAgent_PathFollow>();
    NewAgentEntity.Add<ck::FFragment_CrowdAgent_DesiredVelocity>();
    NewAgentEntity.Add<ck::FFragment_CrowdAgent_FaceAngle>();
    NewAgentEntity.Add<ck::FTag_CrowdAgent_NeedsSetup>();
    NewAgentEntity.Add<ck::FTag_CrowdAgent_Idle>();

    ck::crowd::Verbose(TEXT("CrowdAgent added to [{}] -> [{}] (radius={}, height={})"),
        InOwner, NewAgentEntity, InParams.Get_Radius(), InParams.Get_Height());

    return NewAgentEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_DesiredVelocity(
        const FCk_Handle_CrowdAgent& InHandle)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_DesiredVelocity"), InHandle)
    { return FVector::ZeroVector; }

    return InHandle.Get<ck::FFragment_CrowdAgent_DesiredVelocity>().Get_Velocity();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_TargetYawDegrees(
        const FCk_Handle_CrowdAgent& InHandle)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_TargetYawDegrees"), InHandle)
    { return 0.0f; }

    return FMath::RadiansToDegrees(InHandle.Get<ck::FFragment_CrowdAgent_FaceAngle>().Get_TargetYaw());
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_CrowdAgent_UE, FCk_Handle_CrowdAgent, ck::FFragment_CrowdAgent_Params);

// --------------------------------------------------------------------------------------------------------------------
