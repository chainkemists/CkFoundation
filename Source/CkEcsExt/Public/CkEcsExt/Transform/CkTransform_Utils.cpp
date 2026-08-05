#include "CkTransform_Utils.h"

#include "CkEcsExt/CkEcsExt_Log.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include <Components/MeshComponent.h>
#include <Components/PrimitiveComponent.h>

namespace ck_transform_utils
{
    auto DetermineTeleportType(const USceneComponent* InComponent) -> ETeleportType
    {
        if (const auto PrimitiveComponent = Cast<const UPrimitiveComponent>(InComponent);
            ck::IsValid(PrimitiveComponent) && PrimitiveComponent->IsSimulatingPhysics())
        {
            return ETeleportType::TeleportPhysics;
        }

        return ETeleportType::None;
    }

#if WITH_EDITOR
    auto Get_OnAdded() -> FCk_Transform_OnAdded&
    {
        static auto Delegate = FCk_Transform_OnAdded{};
        return Delegate;
    }

    auto BroadcastAdded(FCk_Handle& InHandle) -> void
    {
        auto Transform = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::IsValid(Transform))
        { Get_OnAdded().Broadcast(Transform); }
    }
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transform_UE::
    Add(
        FCk_Handle& InHandle,
        const FTransform& InInitialTransform,
        ECk_Replication InReplicates)
    -> FCk_Handle_Transform
{
    if (const auto MaybeOwningActor = UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor(InHandle);
        ck::IsValid(MaybeOwningActor))
    {
        const auto RootComponent = MaybeOwningActor->GetRootComponent();
        return AddAndAttachToUnrealComponent(InHandle, RootComponent, InReplicates);
    }

    InHandle.Add<ck::FFragment_Transform>(InInitialTransform);
    InHandle.Add<ck::FFragment_Transform_Previous>(InInitialTransform);
#if WITH_EDITOR
    ck_transform_utils::BroadcastAdded(InHandle);
#endif
    UCk_Utils_Transform_TypeUnsafe_UE::Request_ForceRefresh(InHandle, {});

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

    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_Location>(InHandle, FCk_RepData_Location{InInitialTransform.GetLocation()});
    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_Rotation>(InHandle, FCk_RepData_Rotation{InInitialTransform.GetRotation()});
    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_Scale>(InHandle, FCk_RepData_Scale{InInitialTransform.GetScale3D()});

    return Cast(InHandle);
}

auto
    UCk_Utils_Transform_UE::
    Create(
        FCk_Handle& InOwningEntity,
        const FTransform& InInitialTransform,
        ECk_Replication InReplicates)
    -> FCk_Handle_Transform
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add(NewEntity, InInitialTransform, InReplicates);
}

auto
    UCk_Utils_Transform_UE::
    AddAndAttachToUnrealComponent(
        FCk_Handle& InHandle,
        USceneComponent* InAttachTo,
        ECk_Replication InReplicates)
    -> FCk_Handle_Transform
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAttachTo),
        TEXT("Unable to Add Transform to [{}] and AttachTo because Unreal SceneComponent [{}] is INVALID"), InHandle, InAttachTo)
    { return {}; }

    InHandle.Add<ck::FFragment_Transform_RootComponent>(InAttachTo);
    InHandle.AddOrGet<ck::FFragment_Transform_RootComponentTeleportType>() =
        ck::FFragment_Transform_RootComponentTeleportType{ck_transform_utils::DetermineTeleportType(InAttachTo)};

    if (InAttachTo->Mobility == EComponentMobility::Movable)
    {
        // AddOrGet (not Add): on a CkSnapshot actor-respawn, Request_RebindActor re-runs this against an
        // entity that already carries the (snapshot-restored) Movable tag — an unconditional Add ensures.
        InHandle.AddOrGet<ck::FTag_Transform_Movable>();
    }

    if (InHandle.Has<ck::FFragment_Transform>())
    { return Cast(InHandle); }

    InHandle.Add<ck::FFragment_Transform>(InAttachTo->GetComponentToWorld());
    InHandle.Add<ck::FFragment_Transform_Previous>(InAttachTo->GetComponentToWorld());
#if WITH_EDITOR
    ck_transform_utils::BroadcastAdded(InHandle);
#endif

    if (const auto* AttachToOwner = InAttachTo->GetOwner();
        ck::IsValid(AttachToOwner, ck::IsValid_Policy_NullptrOnly{}) && AttachToOwner->IsReplicatingMovement())
    {
        ck::ecs_extension::VeryVerbose
        (
            TEXT("Skipping creation of Transform Rep Fragment on Entity [{}] because it has an Owning Actor with Replicated Movement"),
            InHandle
        );

        return Cast(InHandle);
    }

    if(InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::ecs_extension::VeryVerbose
        (
            TEXT("Skipping creation of Transform Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return Cast(InHandle);
    }

    const auto ComponentWorldTransform = InAttachTo->GetComponentToWorld();
    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_Location>(InHandle, FCk_RepData_Location{ComponentWorldTransform.GetLocation()});
    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_Rotation>(InHandle, FCk_RepData_Rotation{ComponentWorldTransform.GetRotation()});
    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_Scale>(InHandle, FCk_RepData_Scale{ComponentWorldTransform.GetScale3D()});

    return Cast(InHandle);
}

auto
    UCk_Utils_Transform_UE::
    AddAndAttachToUnrealMesh(
        FCk_Handle& InHandle,
        const UMeshComponent* InAttachTo,
        FName InSocketName,
        ECk_Replication InReplicates)
    -> FCk_Handle_Transform
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAttachTo),
        TEXT("Unable to Add Transform to [{}] and AttachTo because Unreal SceneComponent [{}] is INVALID"), InHandle, InAttachTo)
    { return {}; }

    CK_ENSURE_IF_NOT(InAttachTo->DoesSocketExist(InSocketName),
        TEXT("Socket [{}] does NOT exist on MeshComponent [{}]. "
             "If you wanted to anchor to the component's world transform (no socket), use "
             "AddAndAttachToUnrealComponent instead. If you wanted a specific socket, check "
             "the name for typos."),
        InSocketName, InAttachTo)
    { return {}; }

    InHandle.Add<ck::FFragment_Transform_MeshSocket>(InAttachTo, InSocketName);

    if (InHandle.Has<ck::FFragment_Transform>())
    { return Cast(InHandle); }

    InHandle.Add<ck::FFragment_Transform>(InAttachTo->GetSocketTransform(InSocketName));
    InHandle.Add<ck::FFragment_Transform_Previous>(InAttachTo->GetSocketTransform(InSocketName));
#if WITH_EDITOR
    ck_transform_utils::BroadcastAdded(InHandle);
#endif

    if (const auto* AttachToOwner = InAttachTo->GetOwner();
        ck::IsValid(AttachToOwner, ck::IsValid_Policy_NullptrOnly{}) && AttachToOwner->IsReplicatingMovement())
    {
        ck::ecs_extension::VeryVerbose
        (
            TEXT("Skipping creation of Transform Rep Fragment on Entity [{}] because it has an Owning Actor with Replicated Movement"),
            InHandle
        );

        return Cast(InHandle);
    }

    if(InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::ecs_extension::VeryVerbose
        (
            TEXT("Skipping creation of Transform Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return Cast(InHandle);
    }

    const auto SocketWorldTransform = InAttachTo->GetSocketTransform(InSocketName);
    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_Location>(InHandle, FCk_RepData_Location{SocketWorldTransform.GetLocation()});
    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_Rotation>(InHandle, FCk_RepData_Rotation{SocketWorldTransform.GetRotation()});
    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_Scale>(InHandle, FCk_RepData_Scale{SocketWorldTransform.GetScale3D()});

    return Cast(InHandle);
}

#if WITH_EDITOR
auto
    UCk_Utils_Transform_UE::
    Get_OnAdded()
    -> FCk_Transform_OnAdded&
{
    return ck_transform_utils::Get_OnAdded();
}
#endif

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Transform_UE, FCk_Handle_Transform, ck::FFragment_Transform);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transform_UE::
    Request_SetLocationAndRotation(
        FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetLocationAndRotation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    Request_SetLocation(InHandle, FCk_Request_Transform_SetLocation{InRequest.Get_NewLocation()}.Set_LocalWorld(InRequest.Get_LocalWorld()), {});
    Request_SetRotation(InHandle, FCk_Request_Transform_SetRotation{InRequest.Get_NewRotation()}.Set_LocalWorld(InRequest.Get_LocalWorld()), InDelegate);
}

auto
    UCk_Utils_Transform_UE::
    Request_SetLocation(
        FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetLocation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._LocationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_AddLocationOffset(
        FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_AddLocationOffset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._LocationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_SetRotation(
        FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetRotation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._RotationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_AddRotationOffset(
        FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_AddRotationOffset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._RotationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_ForceRefresh(
        FCk_Handle_Transform& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    const auto Request = FCk_Request_Transform_ForceRefresh{};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._ForceRefreshRequests.Emplace(Request);
}

auto
    UCk_Utils_Transform_UE::
    Request_SetScale(
        FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetScale&  InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    auto& ScaleRequest = InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._ScaleRequests;

    // The slot holds one request, so assigning drops whatever was pending — complete it first or a
    // caller awaiting the superseded request waits forever.
    if (ScaleRequest.IsSet())
    { ScaleRequest->TryFireCompletion(InHandle, ECk_Request_OperationResult::Failed); }

    ScaleRequest = InRequest;
}

auto
    UCk_Utils_Transform_UE::
    Request_SetTransform(
        FCk_Handle_Transform& InHandle,
        const FCk_Request_Transform_SetTransform& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    const auto& NewTransform = InRequest.Get_NewTransform();

    auto& RequestsFragment = InHandle.AddOrGet<ck::FFragment_Transform_Requests>();

    RequestsFragment._LocationRequests.Emplace(FCk_Request_Transform_SetLocation{NewTransform.GetLocation()}.Set_LocalWorld(ECk_LocalWorld::World));
    RequestsFragment._RotationRequests.Emplace(FCk_Request_Transform_SetRotation{NewTransform.GetRotation().Rotator()}.Set_LocalWorld(ECk_LocalWorld::World));

    const auto ScaleRequest = FCk_Request_Transform_SetScale{NewTransform.GetScale3D()}.Set_LocalWorld(ECk_LocalWorld::World);

    if (InDelegate.IsBound())
    { ScaleRequest.Set_CompletionDelegate(InDelegate); }

    if (RequestsFragment._ScaleRequests.IsSet())
    { RequestsFragment._ScaleRequests->TryFireCompletion(InHandle, ECk_Request_OperationResult::Failed); }

    RequestsFragment._ScaleRequests = ScaleRequest;
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
            ck::IsValid(RootComponentFragment.Get_RootComponent()))
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
            ck::IsValid(RootComponentFragment.Get_RootComponent()))
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
            ck::IsValid(RootComponentFragment.Get_RootComponent()))
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
            ck::IsValid(RootComponentFragment.Get_RootComponent()))
        {
            return RootComponentFragment.Get_RootComponent()->GetComponentScale();
        }
    }

    return Get_EntityCurrentTransform(InHandle).GetScale3D();
}

auto
    UCk_Utils_Transform_UE::
    Get_IdentityMatrix()
        -> FTransform
{
    return FTransform::Identity;
}

auto
    UCk_Utils_Transform_UE::
    BindTo_OnUpdate(
        FCk_Handle_Transform& InHandle,
        const FCk_Delegate_Transform_OnUpdate& InDelegate,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Transform
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_TransformUpdate, InHandle, InDelegate, InBehavior, InPostFireBehavior);

    if (NOT InHandle.Has<ck::FFragment_Transform_RootComponent>())
    { return InHandle; }

    if (const auto& RootComponentFragment = InHandle.Get<ck::FFragment_Transform_RootComponent>();
        ck::IsValid(RootComponentFragment.Get_RootComponent()))
    {
        // RootComponent-backed entities are not refreshed every frame by the processors, so the fragment has to
        // be seeded NOW for the Update processor's change detection to have a baseline.
        InHandle.Get<ck::FFragment_Transform>()._Transform = RootComponentFragment.Get_RootComponent()->GetComponentToWorld();
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

    if (ck::UUtils_Signal_TransformUpdate::IsBoundToDelegate(InHandle))
    { return InHandle; }

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
    Apply_SetTransform_DirectWrite(
        ck::FFragment_Transform& InTransformFragment,
        ck::FFragment_Transform_Previous& InPrevTransformFragment,
        const FTransform& InNewTransform)
    -> ECk_TransformComponents
{
    const auto PreviousTransform = InTransformFragment.Get_Transform();
    InPrevTransformFragment = ck::FFragment_Transform_Previous{PreviousTransform};
    InTransformFragment._Transform = InNewTransform;

    auto ComponentsModified = ECk_TransformComponents::None;

    if (NOT InNewTransform.GetLocation().Equals(PreviousTransform.GetLocation()))
    { ComponentsModified |= ECk_TransformComponents::Location; }

    if (NOT InNewTransform.GetRotation().Equals(PreviousTransform.GetRotation()))
    { ComponentsModified |= ECk_TransformComponents::Rotation; }

    if (NOT InNewTransform.GetScale3D().Equals(PreviousTransform.GetScale3D()))
    { ComponentsModified |= ECk_TransformComponents::Scale; }

    InTransformFragment.Set_ComponentsModified(ComponentsModified);

    return ComponentsModified;
}

auto
    UCk_Utils_Transform_UE::
    Request_TransformUpdated(
        FCk_Handle_Transform& InHandle)
    -> void
{
    // Immediate (not queued like Request_ForceRefresh): the tag must land THIS frame so FireSignals sees it
    // later in the same frame's Transform_Finalize group.
    InHandle.AddOrGet<ck::FTag_Transform_Updated>();
}

auto
    UCk_Utils_Transform_UE::
    Request_SetWorldTransformFastPath(
        USceneComponent* InSceneComp,
        const FTransform& InTransform)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSceneComp),
        TEXT("Invalid SceneComponent supplied to Request_SetWorldTransformFastPath"))
    { return; }

    InSceneComp->SetComponentToWorld(InTransform);
    InSceneComp->UpdateBounds();

    if (auto* PrimitiveComp = ::Cast<UPrimitiveComponent>(InSceneComp);
        ck::IsValid(PrimitiveComp))
    {
        auto& BodyInstance = PrimitiveComp->BodyInstance;
        FPhysicsCommand::ExecuteWrite(BodyInstance.ActorHandle, [&](const FPhysicsActorHandle&)
        {
           FPhysicsInterface::SetGlobalPose_AssumesLocked(BodyInstance.ActorHandle, InTransform);
        });
    }

    InSceneComp->MarkRenderTransformDirty();

    ck::algo::ForEachIsValid(InSceneComp->GetAttachChildren(), [&](USceneComponent* InAttachedComp)
    {
        const auto CompWorldTransform = InAttachedComp->GetRelativeTransform() * InTransform;

        Request_SetWorldTransformFastPath(InAttachedComp,CompWorldTransform);
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transform_TypeUnsafe_UE::
    Request_SetLocationAndRotation(
        FCk_Handle& InHandle,
        const FCk_Request_Transform_SetLocationAndRotation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    Request_SetLocation(InHandle, FCk_Request_Transform_SetLocation{InRequest.Get_NewLocation()}.Set_LocalWorld(InRequest.Get_LocalWorld()), {});
    Request_SetRotation(InHandle, FCk_Request_Transform_SetRotation{InRequest.Get_NewRotation()}.Set_LocalWorld(InRequest.Get_LocalWorld()), InDelegate);
}

auto
    UCk_Utils_Transform_TypeUnsafe_UE::
    Request_SetLocation(
        FCk_Handle& InHandle,
        const FCk_Request_Transform_SetLocation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
    const auto TransformHandleIsValid = ck::IsValid(TransformHandle);

    CK_ENSURE_IF_NOT(TransformHandleIsValid, TEXT("Handle [{}] does NOT have Transform"), InHandle) {}
    if (NOT TransformHandleIsValid)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    UCk_Utils_Transform_UE::Request_SetLocation(TransformHandle, InRequest, InDelegate);
}

auto
    UCk_Utils_Transform_TypeUnsafe_UE::
    Request_AddLocationOffset(
        FCk_Handle& InHandle,
        const FCk_Request_Transform_AddLocationOffset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
    const auto TransformHandleIsValid = ck::IsValid(TransformHandle);

    CK_ENSURE_IF_NOT(TransformHandleIsValid, TEXT("Handle [{}] does NOT have Transform"), InHandle) {}
    if (NOT TransformHandleIsValid)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    UCk_Utils_Transform_UE::Request_AddLocationOffset(TransformHandle, InRequest, InDelegate);
}

auto
    UCk_Utils_Transform_TypeUnsafe_UE::
    Request_SetRotation(
        FCk_Handle& InHandle,
        const FCk_Request_Transform_SetRotation& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
    const auto TransformHandleIsValid = ck::IsValid(TransformHandle);

    CK_ENSURE_IF_NOT(TransformHandleIsValid, TEXT("Handle [{}] does NOT have Transform"), InHandle) {}
    if (NOT TransformHandleIsValid)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    UCk_Utils_Transform_UE::Request_SetRotation(TransformHandle, InRequest, InDelegate);
}

auto
    UCk_Utils_Transform_TypeUnsafe_UE::
    Request_AddRotationOffset(
        FCk_Handle& InHandle,
        const FCk_Request_Transform_AddRotationOffset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
    const auto TransformHandleIsValid = ck::IsValid(TransformHandle);

    CK_ENSURE_IF_NOT(TransformHandleIsValid, TEXT("Handle [{}] does NOT have Transform"), InHandle) {}
    if (NOT TransformHandleIsValid)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    UCk_Utils_Transform_UE::Request_AddRotationOffset(TransformHandle, InRequest, InDelegate);
}

auto
    UCk_Utils_Transform_TypeUnsafe_UE::
    Request_ForceRefresh(
        FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
    const auto TransformHandleIsValid = ck::IsValid(TransformHandle);

    CK_ENSURE_IF_NOT(TransformHandleIsValid, TEXT("Handle [{}] does NOT have Transform"), InHandle) {}
    if (NOT TransformHandleIsValid)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    UCk_Utils_Transform_UE::Request_ForceRefresh(TransformHandle, InDelegate);
}

auto
    UCk_Utils_Transform_TypeUnsafe_UE::
    Request_SetScale(
        FCk_Handle& InHandle,
        const FCk_Request_Transform_SetScale& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
    const auto TransformHandleIsValid = ck::IsValid(TransformHandle);

    CK_ENSURE_IF_NOT(TransformHandleIsValid, TEXT("Handle [{}] does NOT have Transform"), InHandle) {}
    if (NOT TransformHandleIsValid)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    UCk_Utils_Transform_UE::Request_SetScale(TransformHandle, InRequest, InDelegate);
}

auto
    UCk_Utils_Transform_TypeUnsafe_UE::
    Request_SetTransform(
        FCk_Handle& InHandle,
        const FCk_Request_Transform_SetTransform& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
    const auto TransformHandleIsValid = ck::IsValid(TransformHandle);

    CK_ENSURE_IF_NOT(TransformHandleIsValid, TEXT("Handle [{}] does NOT have Transform"), InHandle) {}
    if (NOT TransformHandleIsValid)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    UCk_Utils_Transform_UE::Request_SetTransform(TransformHandle, InRequest, InDelegate);
}

auto
    UCk_Utils_Transform_TypeUnsafe_UE::
    Get_EntityCurrentTransform(
        const FCk_Handle& InHandle)
    -> FTransform
{
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
    CK_ENSURE_IF_NOT(ck::IsValid(TransformHandle), TEXT("Handle [{}] does NOT have Transform"), InHandle) { return {}; }
    return UCk_Utils_Transform_UE::Get_EntityCurrentTransform(TransformHandle);
}

auto
    UCk_Utils_Transform_TypeUnsafe_UE::
    Get_EntityCurrentLocation(
        const FCk_Handle& InHandle)
    -> FVector
{
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);

    CK_ENSURE_IF_NOT(ck::IsValid(TransformHandle), TEXT("Handle [{}] does NOT have Transform"), InHandle) { return {}; }
    return UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);
}

auto
    UCk_Utils_Transform_TypeUnsafe_UE::
    Get_EntityCurrentRotation(
        const FCk_Handle& InHandle)
    -> FRotator
{
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
    CK_ENSURE_IF_NOT(ck::IsValid(TransformHandle), TEXT("Handle [{}] does NOT have Transform"), InHandle) { return {}; }
    return UCk_Utils_Transform_UE::Get_EntityCurrentRotation(TransformHandle);
}

auto
    UCk_Utils_Transform_TypeUnsafe_UE::
    Get_EntityCurrentScale(
        const FCk_Handle& InHandle)
    -> FVector
{
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
    CK_ENSURE_IF_NOT(ck::IsValid(TransformHandle), TEXT("Handle [{}] does NOT have Transform"), InHandle) { return {}; }
    return UCk_Utils_Transform_UE::Get_EntityCurrentScale(TransformHandle);
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

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_TransformInterpolation_UE, FCk_Handle_TransformInterpolation, ck::FFragment_TransformInterpolation_Params);

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

