#include "CkProjectile_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
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
        const FCk_Fragment_Projectile_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_Projectile
{
    InHandle.Add<ck::FTag_Projectile>();

    UCk_Utils_Velocity_UE::Add(InHandle, InParams.Get_VelocityParams(), InReplicates);
    UCk_Utils_Acceleration_UE::Add(InHandle, InParams.Get_AccelerationParams(), InReplicates);
    UCk_Utils_AutoReorient_UE::Add(InHandle, InParams.Get_AutoReorientParams());

    UCk_Utils_EulerIntegrator_UE::Request_Start(InHandle);
    UCk_Utils_AutoReorient_UE::Request_Start(InHandle);

    return Cast(InHandle);
}

auto
    UCk_Utils_Projectile_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_Fragment_Projectile_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_Projectile
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    return Add(NewEntity, InParams, InReplicates);
}

auto
    UCk_Utils_Projectile_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_Projectile>();
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
    CK_CALLSTACK_RECORD(ck::FFragment_Projectile_Requests, InHandle);

    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    UCk_Utils_Variables_InstancedStruct_UE::Set(RequestEntity, FGameplayTag::EmptyTag, InOptionalPayload);
    RequestEntity.Add<ck::FFragment_Projectile_Requests>()._Request = InRequest;

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Projectile_OnAimAheadCalculated, RequestEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------
