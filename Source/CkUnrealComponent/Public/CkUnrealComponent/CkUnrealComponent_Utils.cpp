#include "CkUnrealComponent_Utils.h"

#include "CkUnrealComponent/CkUnrealComponent_Fragment.h"
#include "CkUnrealComponent/CkUnrealComponent_Log.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkJolt/StaticWorld/CkJoltStaticWorld_Utils.h"

#include <Components/PrimitiveComponent.h>
#include <Components/SceneComponent.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_actor_component_internal
{
    static auto
    Get_BridgeMap() -> TMap<TWeakObjectPtr<UActorComponent>, FCk_Handle_UnrealComponent>&
    {
        static auto Bridge = TMap<TWeakObjectPtr<UActorComponent>, FCk_Handle_UnrealComponent>{};
        return Bridge;
    }
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_UnrealComponent_UE, FCk_Handle_UnrealComponent,
    ck::FFragment_UnrealComponent_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UnrealComponent_UE::
    Make_Params(
        TSubclassOf<UActorComponent> InComponentClass,
        ECk_UnrealComponent_TickPolicy InTickPolicy,
        FName InDebugName)
    -> FCk_Fragment_UnrealComponent_ParamsData
{
    auto Params = FCk_Fragment_UnrealComponent_ParamsData(InComponentClass);
    Params.Set_TickPolicy(InTickPolicy);
    Params.Set_DebugName(InDebugName);
    return Params;
}

auto
    UCk_Utils_UnrealComponent_UE::
    Make_Params_FromArchetype(
        UActorComponent* InComponentArchetype,
        ECk_UnrealComponent_TickPolicy InTickPolicy,
        FName InDebugName)
    -> FCk_Fragment_UnrealComponent_ParamsData
{
    auto Params = FCk_Fragment_UnrealComponent_ParamsData(InComponentArchetype);
    Params.Set_TickPolicy(InTickPolicy);
    Params.Set_DebugName(InDebugName);
    return Params;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UnrealComponent_UE::
    Add(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_UnrealComponent_ParamsData& InParams)
    -> FCk_Handle_UnrealComponent
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwnerEntity),
        TEXT("Cannot Add UnrealComponent feature to invalid OwnerEntity"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_ComponentClass()),
        TEXT("Cannot Add UnrealComponent feature to [{}] with null ComponentClass"), InOwnerEntity)
    { return {}; }

    ck::unreal_component::VeryVerbose(TEXT("Adding UnrealComponent [{}] to Entity [{}]"),
        InParams.Get_ComponentClass()->GetName(), InOwnerEntity);

    ck::RecordOfUnrealComponents_Utils::AddIfMissing(InOwnerEntity);

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwnerEntity, [&](FCk_Handle InNewEntity)
    {
        InNewEntity.Add<ck::FFragment_UnrealComponent_Params>(InParams);
        InNewEntity.Add<ck::FFragment_UnrealComponent_Current>(InOwnerEntity);
        InNewEntity.Add<ck::FTag_UnrealComponent_NeedsSetup>();

        const auto DebugName = InParams.Get_DebugName().IsNone()
            ? InParams.Get_ComponentClass()->GetName()
            : InParams.Get_DebugName().ToString();
        UCk_Utils_Handle_UE::Set_DebugName(InNewEntity, *ck::Format_UE(TEXT("UnrealComponent: {}"), DebugName));
    });

    auto NewHandle = CastChecked(NewEntity);

    ck::RecordOfUnrealComponents_Utils::Request_Connect(
        InOwnerEntity, NewHandle, ECk_Record_LabelRequirementPolicy::Optional);

    return NewHandle;
}

auto
    UCk_Utils_UnrealComponent_UE::
    Request_Remove(
        FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    const auto UnrealComponentIsValid = ck::IsValid(InUnrealComponent);
    CK_ENSURE_IF_NOT(UnrealComponentIsValid,
        TEXT("Cannot Remove invalid UnrealComponent"))
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    ck::unreal_component::Verbose(TEXT("Requesting Remove for UnrealComponent [{}]"), InUnrealComponent);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InUnrealComponent);

    // Immediate mutation — destroy is initiated synchronously (CkEcs owns the deferred teardown
    // pipeline from here), so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Succeeded);
}

auto
    UCk_Utils_UnrealComponent_UE::
    Request_DisableTransformPush(
        FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_UnrealComponent
{
    const auto UnrealComponentIsValid = ck::IsValid(InUnrealComponent);
    CK_ENSURE_IF_NOT(UnrealComponentIsValid,
        TEXT("Cannot disable transform-push on invalid UnrealComponent"))
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InUnrealComponent;
    }

    InUnrealComponent.AddOrGet<ck::FTag_UnrealComponent_TransformPushDisabled>();

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Succeeded);

    return InUnrealComponent;
}

auto
    UCk_Utils_UnrealComponent_UE::
    Request_EnableTransformPush(
        FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_UnrealComponent
{
    const auto UnrealComponentIsValid = ck::IsValid(InUnrealComponent);
    CK_ENSURE_IF_NOT(UnrealComponentIsValid,
        TEXT("Cannot enable transform-push on invalid UnrealComponent"))
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InUnrealComponent;
    }

    const auto SetupIsComplete = NOT InUnrealComponent.Has<ck::FTag_UnrealComponent_NeedsSetup>();
    CK_ENSURE_IF_NOT(SetupIsComplete,
        TEXT("Cannot enable transform-push on UnrealComponent [{}] before setup completes"), InUnrealComponent)
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InUnrealComponent;
    }

    const auto IsSceneComponent = InUnrealComponent.Has<ck::FTag_UnrealComponent_IsScene>();
    CK_ENSURE_IF_NOT(IsSceneComponent,
        TEXT("Cannot enable transform-push on UnrealComponent [{}] because it does not host a SceneComponent"),
        InUnrealComponent)
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InUnrealComponent;
    }

    // A static-world bake is a collision snapshot. Its existing rebake path is PostTransform, so
    // this synchronous API rejects it rather than moving the Unreal component ahead of its bodies.
    const auto IsBakedIntoStaticWorld = InUnrealComponent.Has<ck::FTag_UnrealComponent_BakedIntoStaticWorld>();
    CK_ENSURE_IF_NOT(NOT IsBakedIntoStaticWorld,
        TEXT("Cannot enable transform-push on static-world-baked UnrealComponent [{}] — remove its baked bodies first"),
        InUnrealComponent)
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InUnrealComponent;
    }

    const auto HasCurrentFragment = InUnrealComponent.Has<ck::FFragment_UnrealComponent_Current>();
    CK_ENSURE_IF_NOT(HasCurrentFragment,
        TEXT("Cannot enable transform-push on UnrealComponent [{}] without a Current fragment"), InUnrealComponent)
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InUnrealComponent;
    }

    const auto& Current = InUnrealComponent.Get<ck::FFragment_UnrealComponent_Current>();
    auto* SceneComponent = ::Cast<USceneComponent>(Current.Get_Component().Get());
    const auto SceneComponentIsValid = ck::IsValid(SceneComponent);
    CK_ENSURE_IF_NOT(SceneComponentIsValid,
        TEXT("Cannot enable transform-push on UnrealComponent [{}] with an invalid SceneComponent"),
        InUnrealComponent)
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InUnrealComponent;
    }

    const auto SceneComponentIsMovable = SceneComponent->Mobility == EComponentMobility::Movable;
    CK_ENSURE_IF_NOT(SceneComponentIsMovable,
        TEXT("Cannot enable transform-push on UnrealComponent [{}] because its SceneComponent is not Movable"),
        InUnrealComponent)
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InUnrealComponent;
    }

    const auto OwningEntity = Current.Get_OwningEntity();
    const auto OwningEntityHasTransform = ck::IsValid(OwningEntity) && UCk_Utils_Transform_UE::Has(OwningEntity);
    CK_ENSURE_IF_NOT(OwningEntityHasTransform,
        TEXT("Cannot enable transform-push on UnrealComponent [{}] because its owning entity has no valid Transform"),
        InUnrealComponent)
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InUnrealComponent;
    }

    const auto OwnerTransform = UCk_Utils_Transform_UE::CastChecked(OwningEntity);

    // FProcessor_UnrealComponent_Setup seeds LastPushedTransform on EVERY scene-component owner,
    // enabled or not, because it is FProcessor_UnrealComponent_PushTransform's view-membership ticket.
    // Its absence therefore means this owner is not in that view: removing the tag below would enable a
    // push that can never run again after the one-shot synchronization, and the component would freeze
    // at this pose for the rest of its life. Refuse, and name it, rather than hand back a Succeeded that
    // silently degrades into a frozen mesh someone debugs from a screenshot.
    const auto OwnerIsPushTracked = OwnerTransform.Has<ck::FFragment_UnrealComponent_LastPushedTransform>();
    CK_ENSURE_IF_NOT(OwnerIsPushTracked,
        TEXT("Cannot enable transform-push on UnrealComponent [{}] because its owning entity [{}] carries no "
             "LastPushedTransform — UnrealComponent Setup never seeded it, so the push processor's view will "
             "never visit this owner and no future move would reach the component"),
        InUnrealComponent, OwningEntity)
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InUnrealComponent;
    }

    const auto TargetTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(OwnerTransform);
    SceneComponent->SetWorldTransform(TargetTransform);

    const auto DidSynchronize = SceneComponent->GetComponentTransform().Equals(TargetTransform);
    CK_ENSURE_IF_NOT(DidSynchronize,
        TEXT("Cannot enable transform-push on UnrealComponent [{}] because its SceneComponent rejected the authoritative transform"),
        InUnrealComponent)
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed);
        return InUnrealComponent;
    }

    // Do this only after every prerequisite and the immediate synchronization succeeded. Failed
    // requests retain the disabled state, so PostTransform cannot take ownership of a stale component.
    if (InUnrealComponent.Has<ck::FTag_UnrealComponent_TransformPushDisabled>())
    { InUnrealComponent.Remove<ck::FTag_UnrealComponent_TransformPushDisabled>(); }
    InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Succeeded);
    return InUnrealComponent;
}

auto
    UCk_Utils_UnrealComponent_UE::
    Get_CanEnableTransformPush(
        const FCk_Handle_UnrealComponent& InUnrealComponent)
    -> bool
{
    const auto HandleIsValid = ck::IsValid(InUnrealComponent);
    if (NOT HandleIsValid ||
        InUnrealComponent.Has<ck::FTag_UnrealComponent_NeedsSetup>() ||
        NOT InUnrealComponent.Has<ck::FTag_UnrealComponent_IsScene>() ||
        InUnrealComponent.Has<ck::FTag_UnrealComponent_BakedIntoStaticWorld>() ||
        NOT InUnrealComponent.Has<ck::FFragment_UnrealComponent_Current>())
    { return false; }

    const auto& Current = InUnrealComponent.Get<ck::FFragment_UnrealComponent_Current>();
    auto* SceneComponent = ::Cast<USceneComponent>(Current.Get_Component().Get());
    const auto SceneComponentIsValid = ck::IsValid(SceneComponent);
    if (NOT SceneComponentIsValid)
    { return false; }

    if (SceneComponent->Mobility != EComponentMobility::Movable)
    { return false; }

    const auto OwningEntity = Current.Get_OwningEntity();
    if (NOT ck::IsValid(OwningEntity) || NOT UCk_Utils_Transform_UE::Has(OwningEntity))
    { return false; }

    // Mirrors Request_EnableTransformPush's LastPushedTransform preflight. This predicate is the
    // documented "can it take transform ownership NOW" answer, so a check the request enforces and
    // this one omits would be a lie that callers gate on.
    return UCk_Utils_Transform_UE::CastChecked(OwningEntity)
        .Has<ck::FFragment_UnrealComponent_LastPushedTransform>();
}

auto
    UCk_Utils_UnrealComponent_UE::
    Request_BakeIntoJoltStaticWorld(
        FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_UnrealComponent
{
    auto* PrimitiveComponent = ::Cast<UPrimitiveComponent>(Get_Component(InUnrealComponent));

    const auto ComponentIsBakeable = ck::IsValid(PrimitiveComponent);
    CK_ENSURE_IF_NOT(ComponentIsBakeable,
        TEXT("Cannot bake UnrealComponent [{}] into the Jolt static world — it hosts no PRIMITIVE component "
             "(not set up yet, torn down, or a non-primitive class). Call this after the component is "
             "created AND configured (an ISM baked before its instances are added bakes nothing)."),
        InUnrealComponent)
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InUnrealComponent;
    }

    const auto NumBodies = UCk_Utils_JoltStaticWorld_UE::Request_BakeComponent(PrimitiveComponent);

    if (NumBodies > 0)
    { InUnrealComponent.AddOrGet<ck::FTag_UnrealComponent_BakedIntoStaticWorld>(); }

    // Zero bodies means the component has no valid collision (already ensured inside extraction
    // where the specific defect is known) — the caller's intent "geometry is in the static world"
    // does not hold, and retrying without fixing the component will not help.
    InDelegate.ExecuteIfBound(InUnrealComponent, NumBodies > 0
        ? ECk_Request_OperationResult::Succeeded
        : ECk_Request_OperationResult::Failed);

    return InUnrealComponent;
}

auto
    UCk_Utils_UnrealComponent_UE::
    Request_RemoveFromJoltStaticWorld(
        FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_UnrealComponent
{
    const auto UnrealComponentIsValid = ck::IsValid(InUnrealComponent);
    CK_ENSURE_IF_NOT(UnrealComponentIsValid,
        TEXT("Cannot remove an invalid UnrealComponent from the Jolt static world"))
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InUnrealComponent;
    }

    if (InUnrealComponent.Has<ck::FTag_UnrealComponent_BakedIntoStaticWorld>())
    {
        InUnrealComponent.Remove<ck::FTag_UnrealComponent_BakedIntoStaticWorld>();

        if (auto* PrimitiveComponent = ::Cast<UPrimitiveComponent>(Get_Component(InUnrealComponent));
            ck::IsValid(PrimitiveComponent))
        { UCk_Utils_JoltStaticWorld_UE::Request_RemoveComponent(PrimitiveComponent); }
    }

    // Removing an unbaked component is an idempotent no-op: the intent "no baked bodies" holds.
    InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Succeeded);

    return InUnrealComponent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UnrealComponent_UE::
    Get_Component(
        const FCk_Handle_UnrealComponent& InUnrealComponent)
    -> UActorComponent*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InUnrealComponent),
        TEXT("Cannot Get_Component on invalid UnrealComponent handle"))
    { return nullptr; }

    return InUnrealComponent.Get<ck::FFragment_UnrealComponent_Current>().Get_Component().Get();
}

auto
    UCk_Utils_UnrealComponent_UE::
    Get_OwningEntity(
        const FCk_Handle_UnrealComponent& InUnrealComponent)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InUnrealComponent),
        TEXT("Cannot Get_OwningEntity on invalid UnrealComponent handle"))
    { return {}; }

    return InUnrealComponent.Get<ck::FFragment_UnrealComponent_Current>().Get_OwningEntity();
}

auto
    UCk_Utils_UnrealComponent_UE::
    TryGet_OwningHandle_FromComponent(
        UActorComponent* InComponent)
    -> FCk_Handle_UnrealComponent
{
    if (ck::Is_NOT_Valid(InComponent))
    { return {}; }

    auto& Bridge = ck_actor_component_internal::Get_BridgeMap();
    if (auto* Found = Bridge.Find(TWeakObjectPtr<UActorComponent>{InComponent});
        Found != nullptr && ck::IsValid(*Found))
    { return *Found; }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UnrealComponent_UE::
    Get_AllHandles(
        const FCk_Handle& InOwnerEntity)
    -> TArray<FCk_Handle_UnrealComponent>
{
    if (ck::Is_NOT_Valid(InOwnerEntity))
    { return {}; }

    if (NOT ck::RecordOfUnrealComponents_Utils::Has(InOwnerEntity))
    { return {}; }

    return ck::RecordOfUnrealComponents_Utils::Get_ValidEntries(InOwnerEntity);
}

auto
    UCk_Utils_UnrealComponent_UE::
    Get_AllComponents(
        const FCk_Handle& InOwnerEntity)
    -> TArray<UActorComponent*>
{
    auto Components = TArray<UActorComponent*>{};
    for (const auto& Handle : Get_AllHandles(InOwnerEntity))
    {
        if (auto* Component = Handle.Get<ck::FFragment_UnrealComponent_Current>().Get_Component().Get();
            ck::IsValid(Component))
        {
            Components.Emplace(Component);
        }
    }
    return Components;
}

auto
    UCk_Utils_UnrealComponent_UE::
    TryGet_HandleByType(
        const FCk_Handle& InOwnerEntity,
        TSubclassOf<UActorComponent> InComponentClass)
    -> FCk_Handle_UnrealComponent
{
    if (ck::Is_NOT_Valid(InComponentClass))
    { return {}; }

    for (const auto& Handle : Get_AllHandles(InOwnerEntity))
    {
        auto* Component = Handle.Get<ck::FFragment_UnrealComponent_Current>().Get_Component().Get();
        if (ck::IsValid(Component) && Component->IsA(InComponentClass))
        { return Handle; }
    }
    return {};
}

auto
    UCk_Utils_UnrealComponent_UE::
    Get_HandlesByType(
        const FCk_Handle& InOwnerEntity,
        TSubclassOf<UActorComponent> InComponentClass)
    -> TArray<FCk_Handle_UnrealComponent>
{
    auto Result = TArray<FCk_Handle_UnrealComponent>{};
    if (ck::Is_NOT_Valid(InComponentClass))
    { return Result; }

    for (const auto& Handle : Get_AllHandles(InOwnerEntity))
    {
        auto* Component = Handle.Get<ck::FFragment_UnrealComponent_Current>().Get_Component().Get();
        if (ck::IsValid(Component) && Component->IsA(InComponentClass))
        { Result.Emplace(Handle); }
    }
    return Result;
}

auto
    UCk_Utils_UnrealComponent_UE::
    TryGet_ComponentByType(
        const FCk_Handle& InOwnerEntity,
        TSubclassOf<UActorComponent> InComponentClass)
    -> UActorComponent*
{
    auto Handle = TryGet_HandleByType(InOwnerEntity, InComponentClass);
    if (ck::Is_NOT_Valid(Handle))
    { return nullptr; }

    return Handle.Get<ck::FFragment_UnrealComponent_Current>().Get_Component().Get();
}

auto
    UCk_Utils_UnrealComponent_UE::
    Get_ComponentsByType(
        const FCk_Handle& InOwnerEntity,
        TSubclassOf<UActorComponent> InComponentClass)
    -> TArray<UActorComponent*>
{
    auto Components = TArray<UActorComponent*>{};
    for (const auto& Handle : Get_HandlesByType(InOwnerEntity, InComponentClass))
    {
        if (auto* Component = Handle.Get<ck::FFragment_UnrealComponent_Current>().Get_Component().Get();
            ck::IsValid(Component))
        {
            Components.Emplace(Component);
        }
    }
    return Components;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UnrealComponent_UE::
    BindTo_OnAdded(
        FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_UnrealComponent_OnAdded& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_UnrealComponent
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_UnrealComponent_OnAdded, InUnrealComponent, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InUnrealComponent;
}

auto
    UCk_Utils_UnrealComponent_UE::
    UnbindFrom_OnAdded(
        FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_UnrealComponent_OnAdded& InDelegate)
    -> FCk_Handle_UnrealComponent
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_UnrealComponent_OnAdded, InUnrealComponent, InDelegate);
    return InUnrealComponent;
}

auto
    UCk_Utils_UnrealComponent_UE::
    BindTo_OnRemoved(
        FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_UnrealComponent_OnRemoved& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_UnrealComponent
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_UnrealComponent_OnRemoved, InUnrealComponent, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InUnrealComponent;
}

auto
    UCk_Utils_UnrealComponent_UE::
    UnbindFrom_OnRemoved(
        FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_UnrealComponent_OnRemoved& InDelegate)
    -> FCk_Handle_UnrealComponent
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_UnrealComponent_OnRemoved, InUnrealComponent, InDelegate);
    return InUnrealComponent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UnrealComponent_UE::
    DoRegisterBridge(
        UActorComponent* InComponent,
        FCk_Handle_UnrealComponent InHandle)
    -> void
{
    if (ck::Is_NOT_Valid(InComponent))
    { return; }

    auto& Bridge = ck_actor_component_internal::Get_BridgeMap();

    // Opportunistically prune entries whose component died without going
    // through DoUnregisterBridge (e.g. PIE teardown ordering) — the map is
    // process-lifetime static and would otherwise grow unbounded.
    for (auto It = Bridge.CreateIterator(); It; ++It)
    {
        if (ck::Is_NOT_Valid(It.Key()))
        { It.RemoveCurrent(); }
    }

    Bridge.Add(TWeakObjectPtr{InComponent}, InHandle);
}

auto
    UCk_Utils_UnrealComponent_UE::
    DoUnregisterBridge(
        UActorComponent* InComponent)
    -> void
{
    if (ck::Is_NOT_Valid(InComponent))
    { return; }

    ck_actor_component_internal::Get_BridgeMap().Remove(TWeakObjectPtr{InComponent});
}

// --------------------------------------------------------------------------------------------------------------------
