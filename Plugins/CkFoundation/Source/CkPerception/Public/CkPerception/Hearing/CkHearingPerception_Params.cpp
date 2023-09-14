#pragma once

#include "CkHearingPerception_Params.h"

#include "CkEnsure/CkEnsure.h"
#include "CkPerception/Hearing/CkHearingPerception_Component.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_HearingPerception_Noise_DebugInfo::
FCk_HearingPerception_Noise_DebugInfo(
        float InLineThickness,
        FColor InDebugLineColor)
    : _LineThickness(InLineThickness)
    , _DebugLineColor(InDebugLineColor)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_HearingPerception_NoiseReceiver_Params::
    FCk_HearingPerception_NoiseReceiver_Params(
        float InHearingModifier,
        ECk_HearingPerception_NoiseFiltering_Policy InNoiseFilteringPolicy)
    : _HearingModifier(InHearingModifier)
    , _NoiseFilteringPolicy(InNoiseFilteringPolicy)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_HearingPerception_NoiseEmitter_Params::
    FCk_HearingPerception_NoiseEmitter_Params(
        float InLoudnessModifier,
        bool InShowDebug,
        FCk_HearingPerception_Noise_DebugInfo InDebugParams)
    : _LoudnessModifier(InLoudnessModifier)
    , _ShowDebug(InShowDebug)
    , _DebugParams(InDebugParams)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_HearingPerception_Listener::
FCk_HearingPerception_Listener(UCk_HearingPerception_NoiseReceiver_ActorComponent_UE* InNoiseReceiverComp)
    : _NoiseReceiverComp(InNoiseReceiverComp)
{
    CK_ENSURE_IF_NOT(ck::IsValid(_NoiseReceiverComp), TEXT("Invalid NoiseReceiver used to create a HearingPerception Listener"))
    { return; }

    _ReceiverOwningActor = _NoiseReceiverComp->GetOwner();
}

auto FCk_HearingPerception_Listener::
operator==(const ThisType& InOther) const
-> bool
{
    return Get_ReceiverOwningActor() == InOther.Get_ReceiverOwningActor() && Get_NoiseReceiverComp() == InOther.Get_NoiseReceiverComp();
}


auto GetTypeHash(const FCk_HearingPerception_Listener& InObj)
-> uint32
{
    return GetTypeHash(InObj.Get_ReceiverOwningActor()) + GetTypeHash(InObj.Get_NoiseReceiverComp());
}

// --------------------------------------------------------------------------------------------------------------------

FCk_HearingPerception_NoiseInfo::
FCk_HearingPerception_NoiseInfo(
        FGameplayTag InNoiseTag,
        float InTravelDistance,
        float InLifetime,
        USoundBase* InNoiseSound)
    : _NoiseTag(InNoiseTag)
    , _TravelDistance(InTravelDistance)
    , _Lifetime(InLifetime)
    , _NoiseSound(InNoiseSound)
{
}

auto
    FCk_HearingPerception_NoiseInfo::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_NoiseTag() == InOther.Get_NoiseTag();
}

auto GetTypeHash(const FCk_HearingPerception_NoiseInfo& InObj)
-> uint32
{
    return GetTypeHash(InObj.Get_NoiseTag());
}

// --------------------------------------------------------------------------------------------------------------------

FCk_HearingPerception_NoiseEvent::
FCk_HearingPerception_NoiseEvent(
        FCk_HearingPerception_NoiseInfo InNoiseInfo,
        FVector InNoiseLocation,
        AActor* InInstigator)
    : _NoiseInfo(InNoiseInfo)
    , _NoiseLocation(InNoiseLocation)
    , _Instigator(InInstigator)
{
}

auto FCk_HearingPerception_NoiseEvent::
operator==(const ThisType& InOther) const
-> bool
{
    return Get_NoiseInfo() == InOther.Get_NoiseInfo() && Get_Instigator() == InOther.Get_Instigator();
}

auto GetTypeHash(const FCk_HearingPerception_NoiseEvent& InObj)
-> uint32
{
    return GetTypeHash(InObj.Get_NoiseInfo()) + GetTypeHash(InObj.Get_Instigator());
}

// --------------------------------------------------------------------------------------------------------------------
