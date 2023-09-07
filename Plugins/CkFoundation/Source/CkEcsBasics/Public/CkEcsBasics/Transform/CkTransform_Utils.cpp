#include "CkTransform_Utils.h"

#include "CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transform_UE::
    Request_SetLocation(
        FCk_Handle                               InHandle,
        const FCk_Request_Transform_SetLocation& InRequest)
    -> void
{
    if (NOT Ensure<ck::type_traits::NonConst>(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._LocationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_AddLocationOffset(
        FCk_Handle                                     InHandle,
        const FCk_Request_Transform_AddLocationOffset& InRequest)
    -> void
{
    if (NOT Ensure<ck::type_traits::NonConst>(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._LocationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_SetRotation(
        FCk_Handle                               InHandle,
        const FCk_Request_Transform_SetRotation& InRequest)
    -> void
{
    if (NOT Ensure<ck::type_traits::NonConst>(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._RotationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_AddRotationOffset(
        FCk_Handle                                     InHandle,
        const FCk_Request_Transform_AddRotationOffset& InRequest)
    -> void
{
    if (NOT Ensure<ck::type_traits::NonConst>(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._RotationRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Transform_UE::
    Request_SetScale(
        FCk_Handle                             InHandle,
        const FCk_Request_Transform_SetScale&  InRequest)
    -> void
{
    if (NOT Ensure<ck::type_traits::NonConst>(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_Transform_Requests>()._ScaleRequests = InRequest;
}

auto
    UCk_Utils_Transform_UE::
    Request_SetTransform(
        FCk_Handle                                InHandle,
        const FCk_Request_Transform_SetTransform& InRequest)
    -> void
{
    const auto& NewTransform = InRequest.Get_NewTransform();

    if (NOT Ensure<ck::type_traits::NonConst>(InHandle))
    { return; }

    auto& RequestsFragment = InHandle.AddOrGet<ck::FFragment_Transform_Requests>();

    RequestsFragment._LocationRequests.Emplace(FCk_Request_Transform_SetLocation{NewTransform.GetLocation()}.Set_RelativeAbsolute(ECk_RelativeAbsolute::Absolute));
    RequestsFragment._RotationRequests.Emplace(FCk_Request_Transform_SetRotation{NewTransform.GetRotation().Rotator()}.Set_RelativeAbsolute(ECk_RelativeAbsolute::Absolute));
    RequestsFragment._ScaleRequests = FCk_Request_Transform_SetScale{NewTransform.GetScale3D()}.Set_RelativeAbsolute(ECk_RelativeAbsolute::Absolute);
}

auto
    UCk_Utils_Transform_UE::
    Request_SetInterpolationGoal_Offset(
        FCk_Handle InHandle,
        FVector    InOffset)
    -> void
{
    auto& NewGoal = InHandle.AddOrGet<ck::FFragment_Transform_NewGoal_Location>();
    NewGoal = ck::FFragment_Transform_NewGoal_Location{InOffset};
}

auto
    UCk_Utils_Transform_UE::
    Get_EntityCurrentTransform(
        FCk_Handle InHandle)
    -> FTransform
{
    if (Has<ck::type_traits::NonConst>(InHandle))
    {
        return InHandle.Get<ck::FFragment_Transform_Current>().Get_Transform();
    }

    if (Has<ck::type_traits::Const>(InHandle))
    {
        return InHandle.Get<ck::FFragment_ImmutableTransform_Current>().Get_Transform();
    }

    CK_ENSURE_FALSE(TEXT("Cannot get the Current Transform of Entity [{}] because it has no Transform component"), InHandle);

    return {};
}

auto
    UCk_Utils_Transform_UE::
    Get_EntityCurrentLocation(
        FCk_Handle InHandle)
    -> FVector
{
    if (Has<ck::type_traits::NonConst>(InHandle))
    {
        return InHandle.Get<ck::FFragment_Transform_Current>().Get_Transform().GetLocation();
    }

    if (Has<ck::type_traits::Const>(InHandle))
    {
        return InHandle.Get<ck::FFragment_ImmutableTransform_Current>().Get_Transform().GetLocation();
    }

    CK_ENSURE_FALSE(TEXT("Cannot get the Current Location of Entity [{}] because it has no Transform component"), InHandle);

    return {};
}

auto
    UCk_Utils_Transform_UE::
    Get_EntityCurrentRotation(
        FCk_Handle InHandle)
    -> FRotator
{
    if (Has<ck::type_traits::NonConst>(InHandle))
    {
        return InHandle.Get<ck::FFragment_Transform_Current>().Get_Transform().GetRotation().Rotator();
    }

    if (Has<ck::type_traits::Const>(InHandle))
    {
        return InHandle.Get<ck::FFragment_ImmutableTransform_Current>().Get_Transform().GetRotation().Rotator();
    }

    CK_ENSURE_FALSE(TEXT("Cannot get the Current Rotation of Entity [{}] because it has no Transform component"), InHandle);

    return {};
}

auto
    UCk_Utils_Transform_UE::
    Get_EntityCurrentScale(
        FCk_Handle InHandle)
    -> FVector
{
    if (Has<ck::type_traits::NonConst>(InHandle))
    {
        return InHandle.Get<ck::FFragment_Transform_Current>().Get_Transform().GetScale3D();
    }

    if (Has<ck::type_traits::Const>(InHandle))
    {
        return InHandle.Get<ck::FFragment_ImmutableTransform_Current>().Get_Transform().GetScale3D();
    }

    CK_ENSURE_FALSE(TEXT("Cannot get the Current Scale of Entity [{}] because it has no Transform component"), InHandle);

    return {};
}

auto
    UCk_Utils_Transform_UE::
    BindTo_OnUpdate(
        FCk_Handle InHandle,
        ECk_Signal_PayloadInFlight InBehavior,
        const FCk_Delegate_Transform_OnUpdate& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    ck::UUtils_Signal_UnrealMulticast_TransformUpdate::Bind(InHandle, InDelegate, InBehavior);
}

auto
    UCk_Utils_Transform_UE::
    DoAdd(
        FCk_Handle InHandle,
        FTransform InInitialTransform,
        FCk_Transform_Interpolation_Settings InSettings)
    -> void
{
    Add<ck::type_traits::NonConst>(InHandle, InInitialTransform, InSettings);
}

auto
    UCk_Utils_Transform_UE::
    DoHas(
        FCk_Handle InHandle)
    -> bool
{
    return Has<ck::type_traits::NonConst>(InHandle);
}

auto
    UCk_Utils_Transform_UE::
    DoEnsure(
        FCk_Handle InHandle)
    -> bool
{
    return Ensure<ck::type_traits::NonConst>(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
