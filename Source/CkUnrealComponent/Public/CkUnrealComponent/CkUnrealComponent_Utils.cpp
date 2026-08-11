#include "CkUnrealComponent_Utils.h"

#include "CkUnrealComponent/CkUnrealComponent_Fragment.h"
#include "CkUnrealComponent/CkUnrealComponent_Log.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkJolt/StaticWorld/CkJoltStaticWorld_Utils.h"

#include <Components/PrimitiveComponent.h>

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
    {}
    if (NOT UnrealComponentIsValid)
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    ck::unreal_component::Verbose(TEXT("Requesting Remove for UnrealComponent [{}]"), InUnrealComponent);

    auto Handle = static_cast<FCk_Handle&>(InUnrealComponent);
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Handle);

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
    {}
    if (NOT UnrealComponentIsValid)
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
    Request_BakeIntoJoltStaticWorld(
        FCk_Handle_UnrealComponent& InUnrealComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_UnrealComponent
{
    auto* PrimitiveComponent = ::Cast<UPrimitiveComponent>(Get_Component(InUnrealComponent));

    const auto ComponentIsBakeable = ck::IsValid(PrimitiveComponent, ck::IsValid_Policy_NullptrOnly{});
    CK_ENSURE_IF_NOT(ComponentIsBakeable,
        TEXT("Cannot bake UnrealComponent [{}] into the Jolt static world — it hosts no PRIMITIVE component "
             "(not set up yet, torn down, or a non-primitive class). Call this after the component is "
             "created AND configured (an ISM baked before its instances are added bakes nothing)."),
        InUnrealComponent)
    {}
    if (NOT ComponentIsBakeable)
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
    {}
    if (NOT UnrealComponentIsValid)
    {
        InDelegate.ExecuteIfBound(InUnrealComponent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InUnrealComponent;
    }

    if (InUnrealComponent.Has<ck::FTag_UnrealComponent_BakedIntoStaticWorld>())
    {
        InUnrealComponent.Remove<ck::FTag_UnrealComponent_BakedIntoStaticWorld>();

        if (auto* PrimitiveComponent = ::Cast<UPrimitiveComponent>(Get_Component(InUnrealComponent));
            ck::IsValid(PrimitiveComponent, ck::IsValid_Policy_NullptrOnly{}))
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
