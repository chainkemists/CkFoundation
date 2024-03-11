#include "CkTransform_Utils.h"

#include "CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transform_UE::
    Request_SetLocation(
        FCk_Handle& InHandle,
        const FCk_Request_Transform_SetLocation& InRequest)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._LocationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_AddLocationOffset(
        FCk_Handle& InHandle,
        const FCk_Request_Transform_AddLocationOffset& InRequest)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._LocationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_SetRotation(
        FCk_Handle& InHandle,
        const FCk_Request_Transform_SetRotation& InRequest)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._RotationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_AddRotationOffset(
        FCk_Handle& InHandle,
        const FCk_Request_Transform_AddRotationOffset& InRequest)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._RotationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_SetScale(
        FCk_Handle& InHandle,
        const FCk_Request_Transform_SetScale&  InRequest)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._ScaleRequests = InRequest;
}

auto
    UCk_Utils_Transform_UE::
    Request_SetTransform(
        FCk_Handle& InHandle,
        const FCk_Request_Transform_SetTransform& InRequest)
    -> void
{
    const auto& NewTransform = InRequest.Get_NewTransform();

    if (NOT Ensure(InHandle))
    { return; }

    auto& RequestsFragment = InHandle.AddOrGet<ck::FFragment_Transform_Requests>();

    RequestsFragment._LocationRequests.Emplace(FCk_Request_Transform_SetLocation{NewTransform.GetLocation()}.Set_LocalWorld(ECk_LocalWorld::World));
    RequestsFragment._RotationRequests.Emplace(FCk_Request_Transform_SetRotation{NewTransform.GetRotation().Rotator()}.Set_LocalWorld(ECk_LocalWorld::World));
    RequestsFragment._ScaleRequests = FCk_Request_Transform_SetScale{NewTransform.GetScale3D()}.Set_LocalWorld(ECk_LocalWorld::World);
}

auto
    UCk_Utils_Transform_UE::
    Request_SetInterpolationGoal_LocationOffset(
        FCk_Handle& InHandle,
        FVector InOffset)
    -> void
{
    auto& NewGoal = InHandle.AddOrGet<ck::FFragment_Transform_NewGoal_Location>();
    NewGoal = ck::FFragment_Transform_NewGoal_Location{InOffset};
}

auto
    UCk_Utils_Transform_UE::
    Request_SetInterpolationGoal_RotationOffset(
        FCk_Handle& InHandle,
        FRotator InOffset)
    -> void
{
    auto& NewGoal = InHandle.AddOrGet<ck::FFragment_Transform_NewGoal_Rotation>();
    NewGoal = ck::FFragment_Transform_NewGoal_Rotation{InOffset};
}

auto
    UCk_Utils_Transform_UE::
    Get_EntityCurrentTransform(
        const FCk_Handle& InHandle)
    -> FTransform
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return InHandle.Get<ck::FFragment_Transform>().Get_Transform();
}

auto
    UCk_Utils_Transform_UE::
    Get_EntityCurrentLocation(
        const FCk_Handle& InHandle)
    -> FVector
{
    return Get_EntityCurrentTransform(InHandle).GetLocation();
}

auto
    UCk_Utils_Transform_UE::
    Get_EntityCurrentRotation(
        const FCk_Handle& InHandle)
    -> FRotator
{
    return Get_EntityCurrentTransform(InHandle).GetRotation().Rotator();
}

auto
    UCk_Utils_Transform_UE::
    Get_EntityCurrentScale(
        const FCk_Handle& InHandle)
    -> FVector
{
    return Get_EntityCurrentTransform(InHandle).GetScale3D();
}

auto
    UCk_Utils_Transform_UE::
    BindTo_OnUpdate(
        FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_Transform_OnUpdate& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    ck::UUtils_Signal_TransformUpdate::Bind(InHandle, InDelegate, InBehavior);
}

auto
    UCk_Utils_Transform_UE::
    UnbindFrom_OnUpdate(
        FCk_Handle& InHandle,
        const FCk_Delegate_Transform_OnUpdate& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    ck::UUtils_Signal_TransformUpdate::Unbind(InHandle, InDelegate);
}

auto
    UCk_Utils_Transform_UE::
    Add(
        FCk_Handle&                     InHandle,
        const FTransform&               InInitialTransform,
        const FCk_Transform_ParamsData& InParams,
        ECk_Replication                 InReplicates)
    -> void
{
    InHandle.Add<ck::FFragment_Transform>(InInitialTransform);
    InHandle.Add<ck::FFragment_Transform_Params>(InParams);

    if (const auto OwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InHandle);
        ck::IsValid(OwningActor))
    {
        if (OwningActor->IsReplicatingMovement())
        {
            ck::ecs_basics::VeryVerbose
            (
                TEXT("Skipping creation of Transform Rep Fragment on Entity [{}] because it has an Owning Actor with Replicated Movement"),
                InHandle
            );

            return;
        }
    }

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::ecs_basics::VeryVerbose
        (
            TEXT("Skipping creation of Transform Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return;
    }

    TryAddReplicatedFragment<UCk_Fragment_Transform_Rep>(InHandle);
}

auto
    UCk_Utils_Transform_UE::
    DoAdd(
        FCk_Handle& InHandle,
        const FTransform& InInitialTransform,
        const FCk_Transform_ParamsData&  InParams,
        ECk_Replication InReplicates)
    -> void
{
    Add(InHandle, InInitialTransform, InParams, InReplicates);
}

auto
    UCk_Utils_Transform_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has_Any<ck::FFragment_Transform>();
}

auto
    UCk_Utils_Transform_UE::
    Ensure(
        const FCk_Handle& InHandle)
        -> bool
{
    // temporary work: be replaced with type-safe handles
    return Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
