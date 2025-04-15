#include "CkRaySense_Utils.h"

#include "CkRaySense/CkRaySense_Fragment.h"
#include "CkRaySense/CkRaySense_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RaySense_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_RaySense_ParamsData& InParams)
    -> FCk_Handle_RaySense
{
    InHandle.Add<ck::FFragment_RaySense_Params>(InParams);
    InHandle.Add<ck::FFragment_RaySense_Current>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_RaySense_UE, FCk_Handle_RaySense,
    ck::FFragment_RaySense_Params, ck::FFragment_RaySense_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RaySense_UE::
    Request_EnableDisable(
        FCk_Handle_RaySense& InHandle,
        const FCk_Request_RaySense_EnableDisable& InRequest)
    -> FCk_Handle_RaySense
{
    InHandle.AddOrGet<ck::FFragment_RaySense_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RaySense_UE::
    BindTo_OnTraceHit(
        FCk_Handle_RaySense& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_RaySense_LineTrace& InDelegate)
    -> FCk_Handle_RaySense
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnRaySenseTraceHit, InHandle, InDelegate, InBehavior, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_RaySense_UE::
    UnbindFrom_OnTraceHit(
        FCk_Handle_RaySense& InHandle,
        const FCk_Delegate_RaySense_LineTrace& InDelegate)
    -> FCk_Handle_RaySense
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnRaySenseTraceHit, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_RaySense_UE::
    DoGet_ShouldIgnoreTraceHit(
        FCk_Handle_RaySense& InHandle,
        const FHitResult& InTraceHit)
    -> bool
{
    const auto& RaySenseParams = InHandle.Get<ck::FFragment_RaySense_Params>();
    const auto& DataToIgnore = RaySenseParams.Get_DataToIgnore();

    if (const auto& HitComponent = InTraceHit.GetComponent();
        ck::IsValid(HitComponent) && DataToIgnore.Get_ComponentsToIgnore().Contains(HitComponent))
    { return true; }

    const auto& HitActor = InTraceHit.GetActor();

    if (ck::Is_NOT_Valid(HitActor))
    { return false; }

    if (DataToIgnore.Get_ActorsToIgnore().Contains(HitActor))
    { return true; }

    const auto& MaybeHitActorEntity = UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(HitActor) ?
            UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(HitActor) : FCk_Handle{};

    if (ck::IsValid(MaybeHitActorEntity) && DataToIgnore.Get_EntitiesToIgnore().Contains(MaybeHitActorEntity))
    { return true; }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------
