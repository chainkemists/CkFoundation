#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkPerception/Hearing/CkHearingPerception_Params.h"

#include <Engine/Classes/Components/ActorComponent.h>

#include "CkHearingPerception_Component.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKPERCEPTION_API UCk_HearingPerception_NoiseReceiver_ActorComponent_UE : public UActorComponent
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(UCk_HearingPerception_NoiseReceiver_ActorComponent_UE);

    UCk_HearingPerception_NoiseReceiver_ActorComponent_UE();

protected:
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UFUNCTION(Client, Reliable /*, WithValidation*/)
    void Client_HandleReportedNoiseEvent(const FCk_HearingPerception_NoiseEvent& InNoise);

private:
    auto DoRegisterListenerToNoiseDispatcher() -> void;
    auto DoUnregisterListenerFromNoiseDispatcher() -> void;

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_HearingPerception_NoiseReceiver_Params _Params;

    UPROPERTY(Transient)
    TMap<FCk_HearingPerception_NoiseEvent, float> _PerceivedNoisesToLifetimeMap;

private:
    UPROPERTY(BlueprintAssignable, meta = (AllowPrivateAccess = true))
    FCk_Delegate_HearingPerception_OnExistingPerceivedNoiseUpdated_MC _OnExistingPerceivedNoiseUpdated;

    UPROPERTY(BlueprintAssignable, meta = (AllowPrivateAccess = true))
    FCk_Delegate_HearingPerception_OnPerceivedNoiseAdded_MC _OnPerceivedNoiseAdded;

    UPROPERTY(BlueprintAssignable, meta = (AllowPrivateAccess = true))
    FCk_Delegate_HearingPerception_OnPerceivedNoiseRemoved_MC _OnPerceivedNoiseRemoved;

public:
    CK_PROPERTY_GET(_Params);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKPERCEPTION_API UCk_HearingPerception_NoiseEmitter_ActorComponent_UE : public UActorComponent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_HearingPerception_NoiseEmitter_ActorComponent_UE);

public:
    UFUNCTION(BlueprintCallable, BlueprintPure = false)
    void TryEmitNoiseAtLocation(const FGameplayTag& InNoiseTag, const FVector& InNoiseLocation) const;

private:
    UFUNCTION(Server, Reliable, BlueprintCallable /*, WithValidation*/)
    void Server_ReportNoiseEvent(const FCk_HearingPerception_NoiseEvent& InNoise) const;

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_HearingPerception_NoiseEmitter_Params _Params;

private:
    UPROPERTY(BlueprintAssignable, meta = (AllowPrivateAccess = true))
    FCk_Delegate_HearingPerception_OnEmitNoiseRequested_MC _OnEmitNoiseRequested;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKPERCEPTION_API UCk_HearingPerception_NoiseDispatcher_ActorComponent_UE : public UActorComponent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_HearingPerception_NoiseDispatcher_ActorComponent_UE);

public:
    auto RegisterListener(const FCk_HearingPerception_Listener& InListener) -> void;
    auto UnregisterListener(const FCk_HearingPerception_Listener& InListener) -> void;

public:
    auto DispatchNoiseToListeners(const FCk_HearingPerception_NoiseEvent& InNoise) -> void;

private:
    UPROPERTY(Transient)
    TSet<FCk_HearingPerception_Listener> _RegisteredListeners;
};

// --------------------------------------------------------------------------------------------------------------------
