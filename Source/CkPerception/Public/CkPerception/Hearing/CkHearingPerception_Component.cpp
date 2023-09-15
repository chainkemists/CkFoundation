#pragma once

#include "CkHearingPerception_Component.h"

#include "CkEnsure/CkEnsure.h"

#include <Engine/World.h>
#include <GameFramework/GameStateBase.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_HearingPerception_NoiseReceiver_ActorComponent_UE::
UCk_HearingPerception_NoiseReceiver_ActorComponent_UE()
{
    this->PrimaryComponentTick.bCanEverTick = true;
    this->PrimaryComponentTick.bStartWithTickEnabled = false;
}

auto UCk_HearingPerception_NoiseReceiver_ActorComponent_UE::
Client_HandleReportedNoiseEvent_Implementation(const FCk_HearingPerception_NoiseEvent& InNoise)
-> void
{
    if (ck::Is_NOT_Valid(InNoise))
    { return; }

    const auto& isNoiseAlreadyPerceived = _PerceivedNoisesToLifetimeMap.Contains(InNoise);

    _PerceivedNoisesToLifetimeMap.Add(InNoise, InNoise.Get_NoiseInfo().Get_Lifetime());

    this->SetComponentTickEnabled(true);

    if (isNoiseAlreadyPerceived)
    {
        _OnExistingPerceivedNoiseUpdated.Broadcast(InNoise);
    }
    else
    {
        _OnPerceivedNoiseAdded.Broadcast(InNoise);
    }
}

auto UCk_HearingPerception_NoiseReceiver_ActorComponent_UE::
OnRegister()
-> void
{
    Super::OnRegister();

    DoRegisterListenerToNoiseDispatcher();
}

auto UCk_HearingPerception_NoiseReceiver_ActorComponent_UE::
OnUnregister()
-> void
{
    DoUnregisterListenerFromNoiseDispatcher();

    Super::OnUnregister();
}

auto UCk_HearingPerception_NoiseReceiver_ActorComponent_UE::
TickComponent(
    float                        DeltaTime,
    ELevelTick                   TickType,
    FActorComponentTickFunction* ThisTickFunction)
-> void
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    for (auto It = _PerceivedNoisesToLifetimeMap.CreateIterator(); It; ++It)
    {
        const auto& remainingNoiseLifetime = It.Value() - DeltaTime;
        const auto& noise = It.Key();
        It.Value() = remainingNoiseLifetime;

        if (remainingNoiseLifetime > 0.0f)
        { continue; }

        _OnPerceivedNoiseRemoved.Broadcast(noise);

        It.RemoveCurrent();
    }

    if (_PerceivedNoisesToLifetimeMap.IsEmpty())
    {
        this->SetComponentTickEnabled(false);
    }
}

auto UCk_HearingPerception_NoiseReceiver_ActorComponent_UE::
DoRegisterListenerToNoiseDispatcher()
-> void
{
    if (NOT IsNetMode(NM_DedicatedServer))
    { return; }

    const auto& world = GetWorld();
    if (ck::Is_NOT_Valid(world))
    { return; }

    const auto& gameState = world->GetGameState();
    if (ck::Is_NOT_Valid(gameState))
    { return; }

    const auto& noiseDispatcherComp = gameState->GetComponentByClass<UCk_HearingPerception_NoiseDispatcher_ActorComponent_UE>();
    CK_ENSURE_IF_NOT(ck::IsValid(noiseDispatcherComp), TEXT("Current GameState does NOT have the required NoiseDispatcher component. Noise Receiver/Emitter will NOT work properly"))
    { return; }

    noiseDispatcherComp->RegisterListener(FCk_HearingPerception_Listener{this});
}

auto UCk_HearingPerception_NoiseReceiver_ActorComponent_UE::
DoUnregisterListenerFromNoiseDispatcher()
-> void
{
    if (NOT IsNetMode(NM_DedicatedServer))
    { return; }

    const auto& world = GetWorld();
    if (ck::Is_NOT_Valid(world))
    { return; }

    const auto& gameState = world->GetGameState();
    if (ck::Is_NOT_Valid(gameState))
    { return; }

    const auto& noiseDispatcherComp = gameState->GetComponentByClass<UCk_HearingPerception_NoiseDispatcher_ActorComponent_UE>();
    CK_ENSURE_IF_NOT(ck::IsValid(noiseDispatcherComp), TEXT("Current GameState does NOT have the required NoiseDispatcher component. Noise Receiver/Emitter will NOT work properly"))
    { return; }

    noiseDispatcherComp->UnregisterListener(FCk_HearingPerception_Listener{this});
}

auto UCk_HearingPerception_NoiseEmitter_ActorComponent_UE::
TryEmitNoiseAtLocation(
    const FGameplayTag& InNoiseTag,
    const FVector& InNoiseLocation) const
-> void
{
    _OnEmitNoiseRequested.Broadcast(InNoiseTag, InNoiseLocation);
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_HearingPerception_NoiseEmitter_ActorComponent_UE::
Server_ReportNoiseEvent_Implementation(const FCk_HearingPerception_NoiseEvent& InNoise) const
-> void
{
    const auto& world = GetWorld();
    if (ck::Is_NOT_Valid(world))
    { return; }

    const auto& gameState = world->GetGameState();
    if (ck::Is_NOT_Valid(gameState))
    { return; }

    const auto& noiseDispatcherComp = gameState->GetComponentByClass<UCk_HearingPerception_NoiseDispatcher_ActorComponent_UE>();
    CK_ENSURE_IF_NOT(ck::IsValid(noiseDispatcherComp), TEXT("Current GameState does NOT have the required NoiseDispatcher component. Noise Receiver/Emitter will NOT work properly"))
    { return; }

    noiseDispatcherComp->DispatchNoiseToListeners(InNoise);
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_HearingPerception_NoiseDispatcher_ActorComponent_UE::
RegisterListener(const FCk_HearingPerception_Listener& InListener)
-> void
{
    if (NOT IsNetMode(NM_DedicatedServer))
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InListener), TEXT("Invalid HearingPerception Listener to Register"))
    { return; }

    _RegisteredListeners.Add(InListener);
}

auto UCk_HearingPerception_NoiseDispatcher_ActorComponent_UE::
UnregisterListener(const FCk_HearingPerception_Listener& InListener)
-> void
{
    if (NOT IsNetMode(NM_DedicatedServer))
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InListener), TEXT("Invalid HearingPerception Listener to Register"))
    { return; }

    _RegisteredListeners.Remove(InListener);
}

auto UCk_HearingPerception_NoiseDispatcher_ActorComponent_UE::
DispatchNoiseToListeners(const FCk_HearingPerception_NoiseEvent& InNoise)
-> void
{
    if (NOT IsNetMode(NM_DedicatedServer))
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InNoise), TEXT("Invalid NoiseEvent reported to NoiseDispatcher"))
    { return; }

    const auto& IsNoisePerceivedByListener = [](const FVector& InListenerLoc, float InListenerHearingModifier, const FVector& InNoiseLoc, float InNoiseTravelDistance) -> bool
    {
        const auto& noiseToListenerDistance = FVector::Dist(InListenerLoc, InNoiseLoc);

        const auto& modifiedNoiseTravelDistance = InNoiseTravelDistance * InListenerHearingModifier;

        return noiseToListenerDistance <= modifiedNoiseTravelDistance;
    };

    const auto& IsListenerInterestedByNoise = [](ECk_HearingPerception_NoiseFiltering_Policy InPolicy, const AActor* InListener, const AActor* InNoiseInstigator) -> bool
    {
        switch (InPolicy)
        {
            case ECk_HearingPerception_NoiseFiltering_Policy::PerceiveAllNoises:
            {
                return true;
            }
            case ECk_HearingPerception_NoiseFiltering_Policy::OnlyPerceiveOtherActorNoises:
            {
                return InListener != InNoiseInstigator;
            }
            case ECk_HearingPerception_NoiseFiltering_Policy::OnlyPerceiveThisActorNoises:
            {
                return InListener == InNoiseInstigator;
            }
            default:
            {
                CK_INVALID_ENUM(InPolicy);
                return false;
            }
        }
    };

    const auto& noiseLocation = InNoise.Get_NoiseLocation();
    const auto& noiseTravelDistance = InNoise.Get_NoiseInfo().Get_TravelDistance();

    for (const auto& listener : _RegisteredListeners)
    {
        const auto& listenerOwningActor = listener.Get_ReceiverOwningActor();
        const auto& noiseReceiverComp = listener.Get_NoiseReceiverComp();
        const auto& noiseReceiverParams = noiseReceiverComp->Get_Params();
        const auto& noiseFilteringPolicy = noiseReceiverParams.Get_NoiseFilteringPolicy();

        if (NOT IsListenerInterestedByNoise(noiseFilteringPolicy, listenerOwningActor, InNoise.Get_Instigator()))
        { continue; }

        const auto& listenerLocation = listenerOwningActor->GetActorLocation();
        const auto& listenerHearingModifier = noiseReceiverParams.Get_HearingModifier();

        if (NOT IsNoisePerceivedByListener(listenerLocation, listenerHearingModifier, noiseLocation, noiseTravelDistance))
        { continue; }

        noiseReceiverComp->Client_HandleReportedNoiseEvent(InNoise);
    }
}

// --------------------------------------------------------------------------------------------------------------------
