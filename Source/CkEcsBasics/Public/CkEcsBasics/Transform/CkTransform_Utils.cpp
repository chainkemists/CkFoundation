#include "CkTransform_Utils.h"

#include "CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transform_UE::
    Add(
        FCk_Handle&                     InHandle,
        const FTransform&               InInitialTransform,
        ECk_Replication                 InReplicates)
    -> FCk_Handle_Transform
{
    InHandle.Add<ck::FFragment_Transform>(InInitialTransform);
    InHandle.Add<ck::FTag_Transform_Setup>();

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

            return Cast(InHandle);
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

        return Cast(InHandle);
    }

    TryAddReplicatedFragment<UCk_Fragment_Transform_Rep>(InHandle);

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(Transform, UCk_Utils_Transform_UE, FCk_Handle_Transform, ck::FFragment_Transform);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transform_UE::
    Request_SetLocation(
        FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetLocation& InRequest)
    -> void
{
    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._LocationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_AddLocationOffset(
        FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_AddLocationOffset& InRequest)
    -> void
{
    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._LocationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_SetRotation(
        FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetRotation& InRequest)
    -> void
{
    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._RotationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_AddRotationOffset(
        FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_AddRotationOffset& InRequest)
    -> void
{
    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._RotationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_SetScale(
        FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetScale&  InRequest)
    -> void
{
    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._ScaleRequests = InRequest;
}

auto
    UCk_Utils_Transform_UE::
    Request_SetTransform(
        FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetTransform& InRequest)
    -> void
{
    const auto& NewTransform = InRequest.Get_NewTransform();

    auto& RequestsFragment = InHandle.AddOrGet<ck::FFragment_Transform_Requests>();

    RequestsFragment._LocationRequests.Emplace(FCk_Request_Transform_SetLocation{NewTransform.GetLocation()}.Set_LocalWorld(ECk_LocalWorld::World));
    RequestsFragment._RotationRequests.Emplace(FCk_Request_Transform_SetRotation{NewTransform.GetRotation().Rotator()}.Set_LocalWorld(ECk_LocalWorld::World));
    RequestsFragment._ScaleRequests = FCk_Request_Transform_SetScale{NewTransform.GetScale3D()}.Set_LocalWorld(ECk_LocalWorld::World);
}

auto
    UCk_Utils_Transform_UE::
    Get_EntityCurrentTransform(
        const FCk_Handle_Transform& InHandle)
    -> FTransform
{
    if (InHandle.Has<ck::FFragment_Transform_RootComponent>())
    {
        auto& RootComponentFragment = InHandle.Get<ck::FFragment_Transform_RootComponent>();

        if (ck::IsValid(RootComponentFragment))
        {
            return RootComponentFragment.Get_RootComponent()->GetComponentToWorld();
        }
    }

    return InHandle.Get<ck::FFragment_Transform>().Get_Transform();
}

auto
    UCk_Utils_Transform_UE::
    Get_EntityCurrentLocation(
        const FCk_Handle_Transform& InHandle)
    -> FVector
{
    if (InHandle.Has<ck::FFragment_Transform_RootComponent>())
    {
        auto& RootComponentFragment = InHandle.Get<ck::FFragment_Transform_RootComponent>();

        if (ck::IsValid(RootComponentFragment))
        {
            return RootComponentFragment.Get_RootComponent()->GetComponentToWorld().GetLocation();
        }
    }

    return Get_EntityCurrentTransform(InHandle).GetLocation();
}

auto
    UCk_Utils_Transform_UE::
    Get_EntityCurrentRotation(
        const FCk_Handle_Transform& InHandle)
    -> FRotator
{
    if (InHandle.Has<ck::FFragment_Transform_RootComponent>())
    {
        auto& RootComponentFragment = InHandle.Get<ck::FFragment_Transform_RootComponent>();

        if (ck::IsValid(RootComponentFragment))
        {
            return RootComponentFragment.Get_RootComponent()->GetComponentToWorld().GetRotation().Rotator();
        }
    }

    return Get_EntityCurrentTransform(InHandle).GetRotation().Rotator();
}

auto
    UCk_Utils_Transform_UE::
    Get_EntityCurrentScale(
        const FCk_Handle_Transform& InHandle)
    -> FVector
{
    if (InHandle.Has<ck::FFragment_Transform_RootComponent>())
    {
        auto& RootComponentFragment = InHandle.Get<ck::FFragment_Transform_RootComponent>();

        if (ck::IsValid(RootComponentFragment))
        {
            return RootComponentFragment.Get_RootComponent()->GetComponentToWorld().GetScale3D();
        }
    }

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
    ck::UUtils_Signal_TransformUpdate::Bind(InHandle, InDelegate, InBehavior);
}

auto
    UCk_Utils_Transform_UE::
    UnbindFrom_OnUpdate(
        FCk_Handle& InHandle,
        const FCk_Delegate_Transform_OnUpdate& InDelegate)
    -> void
{
    ck::UUtils_Signal_TransformUpdate::Unbind(InHandle, InDelegate);
}

auto
    UCk_Utils_Transform_UE::
    DoAdd(
        FCk_Handle& InHandle,
        const FTransform& InInitialTransform,
        ECk_Replication InReplicates)
    -> void
{
    Add(InHandle, InInitialTransform, InReplicates);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TransformInterpolation_UE::
    Add(
        FCk_Handle&                                 InHandle,
        const FCk_Transform_Interpolation_Settings& InParams)
    -> FCk_Handle_TransformInterpolation
{
    InHandle.Add<ck::FFragment_TransformInterpolation_Params>(FCk_TransformInterpolation_ParamsData{InParams});
    return ck::StaticCast<FCk_Handle_TransformInterpolation>(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(TransformInterpolation, UCk_Utils_TransformInterpolation_UE,
    FCk_Handle_TransformInterpolation, ck::FFragment_Transform_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TransformInterpolation_UE::
    Request_SetInterpolationGoal_LocationOffset(
        FCk_Handle_TransformInterpolation& InHandle,
        FVector InOffset)
    -> void
{
    auto& NewGoal = InHandle.AddOrGet<ck::FFragment_Transform_NewGoal_Location>();
    NewGoal = ck::FFragment_Transform_NewGoal_Location{InOffset};
}

auto
    UCk_Utils_TransformInterpolation_UE::
    Request_SetInterpolationGoal_RotationOffset(
        FCk_Handle_TransformInterpolation& InHandle,
        FRotator InOffset)
    -> void
{
    auto& NewGoal = InHandle.AddOrGet<ck::FFragment_Transform_NewGoal_Rotation>();
    NewGoal = ck::FFragment_Transform_NewGoal_Rotation{InOffset};
}

// --------------------------------------------------------------------------------------------------------------------

