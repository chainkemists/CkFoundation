#include "CkDeferredEntity_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_DeferredEntity_UE, FCk_Handle_DeferredEntity, ck::FTag_DeferredEntity);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DeferredEntity_UE::
    Create(
        const FCk_Handle& InContextEntity)
    -> FCk_Handle_DeferredEntity
{
    CK_ENSURE_IF_NOT(ck::IsValid(InContextEntity),
        TEXT("ContextEntity is invalid when trying to create DeferredEntity"))
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InContextEntity);
    return Add(NewEntity);
}

auto
    UCk_Utils_DeferredEntity_UE::
    Add(
        FCk_Handle& InHandle)
    -> FCk_Handle_DeferredEntity
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Handle is invalid when trying to add DeferredEntity feature"))
    { return {}; }

    InHandle.Add<ck::FTag_DeferredEntity>();

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    UCk_Utils_Handle_UE::Set_DebugName(InHandle, TEXT("Deferred Entity"));
#endif

    return ck::StaticCast<FCk_Handle_DeferredEntity>(InHandle);
}

auto
    UCk_Utils_DeferredEntity_UE::
    Get_IsDeferred(
        const FCk_Handle_DeferredEntity& InDeferredEntity)
    -> bool
{
    if (ck::Is_NOT_Valid(InDeferredEntity))
    { return false; }

    return InDeferredEntity.Has<ck::FTag_DeferredEntity>();
}

auto
    UCk_Utils_DeferredEntity_UE::
    Get_PendingCount(
        const FCk_Handle_DeferredEntity& InDeferredEntity)
    -> int32
{
    if (NOT Get_IsDeferred(InDeferredEntity))
    { return 0; }

    const auto& DeferredTag = InDeferredEntity.Get<ck::FTag_DeferredEntity>();
    return DeferredTag.Get_Count();
}

auto
    UCk_Utils_DeferredEntity_UE::
    Request_CompleteSetup(
         FCk_Handle_DeferredEntity& InDeferredEntity)
    -> void
{
    CK_ENSURE_IF_NOT(Get_IsDeferred(InDeferredEntity),
        TEXT("Entity [{}] is not a deferred entity. Cannot complete setup."), InDeferredEntity)
    { return; }

    ck::UUtils_Signal_OnDeferredEntitySetupComplete::Broadcast(InDeferredEntity, ck::MakePayload(InDeferredEntity));

    InDeferredEntity.Remove<ck::FTag_DeferredEntity>();

    if (NOT InDeferredEntity.Has<ck::FTag_DeferredEntity>())
    {
        ck::UUtils_Signal_OnDeferredEntityFullyComplete::Broadcast(InDeferredEntity, ck::MakePayload(InDeferredEntity));
    }
}

auto
    UCk_Utils_DeferredEntity_UE::
    BindTo_OnSetupComplete(
        FCk_Handle_DeferredEntity& InDeferredEntity,
        const FCk_Delegate_DeferredEntity_OnComplete& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> void
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnDeferredEntitySetupComplete, InDeferredEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
}

auto
    UCk_Utils_DeferredEntity_UE::
    BindTo_OnFullyComplete(
        FCk_Handle_DeferredEntity& InDeferredEntity,
        const FCk_Delegate_DeferredEntity_OnFullyComplete& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> void
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnDeferredEntityFullyComplete, InDeferredEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
}

auto
    UCk_Utils_DeferredEntity_UE::
    UnbindFrom_OnSetupComplete(
        FCk_Handle_DeferredEntity& InDeferredEntity,
        const FCk_Delegate_DeferredEntity_OnComplete& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnDeferredEntitySetupComplete::Unbind(InDeferredEntity, InDelegate);
}

auto
    UCk_Utils_DeferredEntity_UE::
    UnbindFrom_OnFullyComplete(
        FCk_Handle_DeferredEntity& InDeferredEntity,
        const FCk_Delegate_DeferredEntity_OnFullyComplete& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnDeferredEntityFullyComplete::Unbind(InDeferredEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------