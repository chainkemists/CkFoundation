#include "CkProjectile_Utils.h"

#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"
#include "CkPhysics/AutoReorient/CkAutoReorient_Utils.h"
#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Utils.h"
#include "CkProjectile/CkProjectile_Fragment.h"
#include "CkVariables/CkUnrealVariables_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Projectile_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Projectile_ParamsData& InParams)
    -> void
{
    UCk_Utils_Velocity_UE::Add(InHandle, InParams.Get_VelocityParams());
    UCk_Utils_Acceleration_UE::Add(InHandle, InParams.Get_AccelerationParams());
    UCk_Utils_AutoReorient_UE::Add(InHandle, InParams.Get_AutoReorientParams());

    UCk_Utils_EulerIntegrator_UE::Request_Start(InHandle);
    UCk_Utils_AutoReorient_UE::Request_Start(InHandle);
}

auto
    UCk_Utils_Projectile_UE::
    Request_CalculateAimAhead(
        const FCk_Handle& InHandle,
        const FCk_Request_Projectile_CalculateAimAhead& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_Projectile_OnAimAheadCalculated& InDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    UCk_Utils_Variables_InstancedStruct_UE::Set(RequestEntity, FGameplayTag::EmptyTag, InOptionalPayload);
    RequestEntity.Add<ck::FFragment_Projectile_Requests>()._Request = InRequest;

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Projectile_OnAimAheadCalculated, RequestEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------
