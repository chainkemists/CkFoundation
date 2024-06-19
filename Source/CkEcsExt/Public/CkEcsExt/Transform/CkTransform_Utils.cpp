#include "CkTransform_Utils.h"

#include "CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transform_UE::
    Add(
        FCk_Handle& InHandle,
        const FTransform& InInitialTransform,
        ECk_Replication InReplicates)
    -> FCk_Handle_Transform
{
    InHandle.Add<ck::FFragment_Transform>(InInitialTransform);

    if (UCk_Utils_OwningActor_UE::Has(InHandle))
    {
        if (const auto OwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InHandle);
            ck::IsValid(OwningActor))
        {
            const auto RootComponent = OwningActor->GetRootComponent();
            InHandle.Add<ck::FFragment_Transform_RootComponent>(RootComponent);

            if (OwningActor->IsReplicatingMovement())
            {
                ck::ecs_extension::VeryVerbose
                (
                    TEXT("Skipping creation of Transform Rep Fragment on Entity [{}] because it has an Owning Actor with Replicated Movement"),
                    InHandle
                );

                return Cast(InHandle);
            }
        }
    }

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::ecs_extension::VeryVerbose
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
        if (auto& RootComponentFragment = InHandle.Get<ck::FFragment_Transform_RootComponent>();
            ck::IsValid(RootComponentFragment))
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
        if (auto& RootComponentFragment = InHandle.Get<ck::FFragment_Transform_RootComponent>();
            ck::IsValid(RootComponentFragment))
        {
            return RootComponentFragment.Get_RootComponent()->GetComponentLocation();
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
        if (auto& RootComponentFragment = InHandle.Get<ck::FFragment_Transform_RootComponent>();
            ck::IsValid(RootComponentFragment))
        {
            return RootComponentFragment.Get_RootComponent()->GetComponentRotation();
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
        if (auto& RootComponentFragment = InHandle.Get<ck::FFragment_Transform_RootComponent>();
            ck::IsValid(RootComponentFragment))
        {
            return RootComponentFragment.Get_RootComponent()->GetComponentScale();
        }
    }

    return Get_EntityCurrentTransform(InHandle).GetScale3D();
}

auto
    UCk_Utils_Transform_UE::
    BindTo_OnUpdate(
        FCk_Handle_Transform& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Transform_OnUpdate& InDelegate)
    -> FCk_Handle_Transform
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_TransformUpdate, InHandle, InDelegate, InBehavior, InPostFireBehavior);

    if (NOT InHandle.Has<ck::FFragment_Transform_RootComponent>())
    { return InHandle; }

    if (InHandle.Has<ck::FTag_Transform_NeedsUpdate>())
    { return InHandle; }

    if (auto& RootComponentFragment = InHandle.Get<ck::FFragment_Transform_RootComponent>();
        ck::IsValid(RootComponentFragment))
    {
        // Need to update the transform value in the fragment since when a RootComponent exists, we don't update it
        // every frame through the processors. We need to set the current value NOW so that we can determine if it has changed
        // when the Update processor is ticked
        InHandle.Get<ck::FFragment_Transform>()._Transform = RootComponentFragment.Get_RootComponent()->GetComponentToWorld();
        InHandle.Add<ck::FTag_Transform_NeedsUpdate>();
    }

    return InHandle;
}

auto
    UCk_Utils_Transform_UE::
    UnbindFrom_OnUpdate(
        FCk_Handle_Transform& InHandle,
        const FCk_Delegate_Transform_OnUpdate& InDelegate)
    -> FCk_Handle_Transform
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_TransformUpdate, InHandle, InDelegate);

    if (NOT InHandle.Has<ck::FFragment_Transform_RootComponent>())
    { return InHandle; }

    if (ck::UUtils_Signal_TransformUpdate::IsBoundToMulticast(InHandle))
    { return InHandle; }

    InHandle.Remove<ck::FTag_Transform_NeedsUpdate>();

    return InHandle;
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

auto
    UCk_Utils_Transform_UE::
    Request_TransformUpdated(
        FCk_Handle_Transform& InHandle)
    -> void
{
    InHandle.AddOrGet<ck::FTag_Transform_Updated>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TransformInterpolation_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Transform_Interpolation_Settings& InParams)
    -> FCk_Handle_TransformInterpolation
{
    InHandle.Add<ck::FFragment_TransformInterpolation_Params>(FCk_TransformInterpolation_ParamsData{InParams});
    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(TransformInterpolation, UCk_Utils_TransformInterpolation_UE,
    FCk_Handle_TransformInterpolation, ck::FFragment_TransformInterpolation_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TransformInterpolation_UE::
    Request_SetInterpolationGoal_LocationOffset(
        FCk_Handle_TransformInterpolation& InHandle,
        FVector InOffset)
    -> void
{
    auto& NewGoal = InHandle.AddOrGet<ck::FFragment_TransformInterpolation_NewGoal_Location>();
    NewGoal = ck::FFragment_TransformInterpolation_NewGoal_Location{InOffset};
}

auto
    UCk_Utils_TransformInterpolation_UE::
    Request_SetInterpolationGoal_RotationOffset(
        FCk_Handle_TransformInterpolation& InHandle,
        FRotator InOffset)
    -> void
{
    auto& NewGoal = InHandle.AddOrGet<ck::FFragment_TransformInterpolation_NewGoal_Rotation>();
    NewGoal = ck::FFragment_TransformInterpolation_NewGoal_Rotation{InOffset};
}

// --------------------------------------------------------------------------------------------------------------------

