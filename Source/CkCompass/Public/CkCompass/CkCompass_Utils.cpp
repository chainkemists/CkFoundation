#include "CkCompass_Utils.h"

#include "CkCompass/CkCompass_Log.h"

#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Compass_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Compass_ParamsData& InParams)
    -> FCk_Handle_Compass
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle. Unable to Add the Compass feature"))
    { return {}; }

    CK_ENSURE_IF_NOT(NOT Has(InHandle),
        TEXT("Handle [{}] already has the Compass feature. An entity hosts at most ONE compass"),
        InHandle)
    { return Cast(InHandle); }

    InHandle.Add<ck::FFragment_Compass_Params>(InParams);
    InHandle.Add<ck::FFragment_Compass_Current>();
    InHandle.Add<ck::FTag_Compass_NeedsSetup>();

    return ck::StaticCast<FCk_Handle_Compass>(InHandle);
}

auto
    UCk_Utils_Compass_UE::
    Create(
        FCk_Handle& InLifetimeOwner,
        const FCk_Fragment_Compass_ParamsData& InParams)
    -> FCk_Handle_Compass
{
    CK_ENSURE_IF_NOT(ck::IsValid(InLifetimeOwner), TEXT("Invalid Lifetime Owner supplied to Compass Create"))
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);

    auto NewCompassEntity = Add(NewEntity, InParams);

    if (ck::Is_NOT_Valid(NewCompassEntity))
    { return {}; }

    // Standalone child: the lifetime owner is the observer (the entity whose position/view drives the heading)
    Request_SetObserver(NewCompassEntity, InLifetimeOwner);

    return NewCompassEntity;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Compass_UE, FCk_Handle_Compass, ck::FFragment_Compass_Current, ck::FFragment_Compass_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Compass_UE::
    Get_Heading(
        const FCk_Handle_Compass& InCompass)
    -> float
{
    return InCompass.Get<ck::FFragment_Compass_Current>().Get_HeadingDegrees();
}

auto
    UCk_Utils_Compass_UE::
    Get_ArcDegrees(
        const FCk_Handle_Compass& InCompass)
    -> float
{
    return InCompass.Get<ck::FFragment_Compass_Params>().Get_ArcDegrees();
}

auto
    UCk_Utils_Compass_UE::
    Get_Observer(
        const FCk_Handle_Compass& InCompass)
    -> FCk_Handle
{
    return InCompass.Get<ck::FFragment_Compass_Current>().Get_Observer();
}

auto
    UCk_Utils_Compass_UE::
    Get_Entries(
        const FCk_Handle_Compass& InCompass)
    -> TArray<FCk_Compass_Entry>
{
    return InCompass.Get<ck::FFragment_Compass_Current>().Get_Entries();
}

auto
    UCk_Utils_Compass_UE::
    Get_CardinalDirection(
        float InHeadingDegrees)
    -> ECk_CardinalAndOrdinalDirection
{
    return UCk_Utils_Vector2_UE::Get_ClosestCardinalAndOrdinalDirection_ByHeading(InHeadingDegrees);
}

auto
    UCk_Utils_Compass_UE::
    Get_NormalizedArcOffset(
        float InSignedBearingDegrees,
        float InArcDegrees)
    -> float
{
    const auto HalfArc = InArcDegrees * 0.5f;

    if (HalfArc <= 0.0f)
    { return 0.0f; }

    return FMath::Clamp(InSignedBearingDegrees / HalfArc, -1.0f, 1.0f);
}

auto
    UCk_Utils_Compass_UE::
    Get_IsOutsideArc(
        float InSignedBearingDegrees,
        float InArcDegrees)
    -> bool
{
    return FMath::Abs(InSignedBearingDegrees) > InArcDegrees * 0.5f;
}

auto
    UCk_Utils_Compass_UE::
    Get_RangeFadeAlpha(
        float InDistance,
        float InMinVisibleRange,
        float InMaxVisibleRange,
        float InFadeBandCm)
    -> float
{
    if (InFadeBandCm <= 0.0f)
    { return 1.0f; }

    auto Alpha = 1.0f;

    if (InMinVisibleRange > 0.0f)
    { Alpha = FMath::Min(Alpha, (InDistance - InMinVisibleRange) / InFadeBandCm); }

    if (InMaxVisibleRange > 0.0f)
    { Alpha = FMath::Min(Alpha, (InMaxVisibleRange - InDistance) / InFadeBandCm); }

    return FMath::Clamp(Alpha, 0.0f, 1.0f);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Compass_UE::
    Request_SetCategoryFilter(
        FCk_Handle_Compass& InCompass,
        const FCk_Request_Compass_SetCategoryFilter& InRequest)
    -> FCk_Handle_Compass
{
    CK_CALLSTACK_RECORD(ck::FFragment_Compass_Requests, InCompass);

    InCompass.AddOrGet<ck::FFragment_Compass_Requests>()._Requests.Emplace(InRequest);

    return InCompass;
}

auto
    UCk_Utils_Compass_UE::
    Request_SetManualHeading(
        FCk_Handle_Compass& InCompass,
        float InHeadingDegrees)
    -> FCk_Handle_Compass
{
    CK_CALLSTACK_RECORD(ck::FFragment_Compass_Requests, InCompass);

    InCompass.AddOrGet<ck::FFragment_Compass_Requests>()._Requests.Emplace(
        FCk_Request_Compass_SetManualHeading{InHeadingDegrees});

    return InCompass;
}

auto
    UCk_Utils_Compass_UE::
    Request_SetObserver(
        FCk_Handle_Compass& InCompass,
        FCk_Handle InObserver)
    -> FCk_Handle_Compass
{
    CK_CALLSTACK_RECORD(ck::FFragment_Compass_Requests, InCompass);

    InCompass.AddOrGet<ck::FFragment_Compass_Requests>()._Requests.Emplace(
        FCk_Request_Compass_SetObserver{InObserver});

    return InCompass;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Compass_UE::
    BindTo_OnEntryAppeared(
        FCk_Handle_Compass& InCompass,
        const FCk_Delegate_Compass_EntryAppeared& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Compass
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnCompassEntryAppeared, InCompass, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InCompass;
}

auto
    UCk_Utils_Compass_UE::
    BindTo_OnEntryDisappeared(
        FCk_Handle_Compass& InCompass,
        const FCk_Delegate_Compass_EntryDisappeared& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Compass
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnCompassEntryDisappeared, InCompass, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InCompass;
}

auto
    UCk_Utils_Compass_UE::
    UnbindFrom_OnEntryAppeared(
        FCk_Handle_Compass& InCompass,
        const FCk_Delegate_Compass_EntryAppeared& InDelegate)
    -> FCk_Handle_Compass
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnCompassEntryAppeared, InCompass, InDelegate);

    return InCompass;
}

auto
    UCk_Utils_Compass_UE::
    UnbindFrom_OnEntryDisappeared(
        FCk_Handle_Compass& InCompass,
        const FCk_Delegate_Compass_EntryDisappeared& InDelegate)
    -> FCk_Handle_Compass
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnCompassEntryDisappeared, InCompass, InDelegate);

    return InCompass;
}

// --------------------------------------------------------------------------------------------------------------------