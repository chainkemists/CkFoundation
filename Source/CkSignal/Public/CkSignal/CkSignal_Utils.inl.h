#pragma once

#include "CkSignal_Utils.inl.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedSignal>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Has(
            FCk_Handle InHandle)
        -> bool
    {
        return InHandle.Has<SignalType>();
    }

    template <typename T_DerivedSignal>
    template <typename ... T_Args>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Broadcast(
            FCk_Handle InHandle,
            TPayload<T_Args...>&& InPayload)
    {
        if (NOT InHandle.Has<SignalType>())
        { return; }

        auto& Signal = InHandle.Get<SignalType>();
        const auto Invoker = [&Signal](auto&&... InArgs)
        {
            Signal._Invoke_Signal.publish(InArgs...);
            Signal._Payload.Emplace(std::make_tuple(std::forward<T_Args>(InArgs)...));
        };

        std::apply(Invoker, InPayload.Payload);
        InHandle.Add<typename SignalType::FTag_PayloadInFlight>();
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, ECk_Signal_BindingPolicy T_PayloadInFlightBehavior>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Bind(
            FCk_Handle InHandle)
    {
        const auto BroadcastIfPayloadInFlight = [&]<typename... T_Args>(std::tuple<T_Args...> InPayload)
        {
            if constexpr (T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::IgnorePayloadInFlight)
            { return; }

            auto TempDelegate = SignalType::template DelegateType{};
            TempDelegate.template connect<T_Candidate>();
            std::apply(TempDelegate, InPayload);
        };

        auto& Signal = InHandle.AddOrGet<SignalType>();

        if (ck::IsValid(Signal._Payload))
        { BroadcastIfPayloadInFlight(*Signal._Payload); }

        return Signal._Invoke_Sink.template connect<T_Candidate>();
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, ECk_Signal_BindingPolicy T_PayloadInFlightBehavior, typename T_Instance>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Bind(
            T_Instance&& InInstance,
            FCk_Handle InHandle)
    {
        const auto BroadcastIfPayloadInFlight = [&]<typename... T_Args>(std::tuple<T_Args...> InPayload)
        {
            if (T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::IgnorePayloadInFlight)
            { return; }


            auto TempDelegate = SignalType::template DelegateType{};
            TempDelegate.template connect<T_Candidate>(InInstance);
            std::apply(TempDelegate, InPayload);
        };

        auto& Signal = InHandle.AddOrGet<SignalType>();

        if (ck::IsValid(Signal._Payload))
        { BroadcastIfPayloadInFlight(*Signal._Payload); }

        return Signal._Invoke_Sink.template connect<T_Candidate>(InInstance);
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Bind(
            FCk_Handle InHandle,
            ECk_Signal_BindingPolicy InPayloadInFlightBehavior)
    {
        using ReturnType = decltype(Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight>(InHandle));

        switch(InPayloadInFlightBehavior)
        {
            case ECk_Signal_BindingPolicy::FireIfPayloadInFlight:
                return Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight>(InHandle);
            case ECk_Signal_BindingPolicy::IgnorePayloadInFlight:
                return Bind<T_Candidate, ECk_Signal_BindingPolicy::IgnorePayloadInFlight>(InHandle);
            default:
                CK_INVALID_ENUM(InPayloadInFlightBehavior);
                break;
        }

        return ReturnType{};
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, typename T_Instance>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Bind(
            T_Instance&& InInstance,
            FCk_Handle InHandle,
            ECk_Signal_BindingPolicy InPayloadInFlightBehavior)
    {
        using ReturnType = decltype(Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight>(std::forward<T_Instance>(InInstance), InHandle));

        switch(InPayloadInFlightBehavior)
        {
            case ECk_Signal_BindingPolicy::FireIfPayloadInFlight:
                return Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight>(std::forward<T_Instance>(InInstance), InHandle);
            case ECk_Signal_BindingPolicy::IgnorePayloadInFlight:
                return Bind<T_Candidate, ECk_Signal_BindingPolicy::IgnorePayloadInFlight>(std::forward<T_Instance>(InInstance), InHandle);
            default:
                CK_INVALID_ENUM(InPayloadInFlightBehavior);
                break;
        }

        return ReturnType{};
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Unbind(
            FCk_Handle InHandle)
    {
        auto& Signal = InHandle.Get<SignalType>();
        Signal._Invoke_Sink.template disconnect<T_Candidate>();
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, typename T_Instance>
    auto
    TUtils_Signal<T_DerivedSignal>::
        Unbind(
            T_Instance&& InInstance,
            FCk_Handle InHandle)
    {
        auto& Signal = InHandle.Get<SignalType>();
        Signal._Invoke_Sink.template disconnect<T_Candidate>();
    }

    // --------------------------------------------------------------------------------------------------------------------
}

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedSignal, typename T_DerivedSignal_Unreal>
    template <ECk_Signal_BindingPolicy T_PayloadInFlightBehavior>
    auto
        TUtils_Signal_UnrealMulticast<T_DerivedSignal, T_DerivedSignal_Unreal>::
        Bind(
            FCk_Handle InHandle,
            UnrealDynamicDelegateType InDelegate)
    {
        auto& Signal = InHandle.AddOrGet<SignalType>();
        auto& UnrealMulticast = InHandle.AddOrGet<T_DerivedSignal_Unreal>();

        CK_ENSURE_IF_NOT(NOT UnrealMulticast._Multicast.Contains(InDelegate),
                         TEXT("The Unreal Multicast already has the Delegate [{}] bound.[{}]"), InDelegate.GetFunctionName().ToString(), ck::Context(InHandle))
        { return; }

        UnrealMulticast._Multicast.AddUnique(InDelegate);

        if (NOT UnrealMulticast._Connection)
        {
            //SignalType s;
            //s._Invoke_Sink.connect<&T_DerivedSignal_Unreal::DoBroadcast>(UnrealMulticast);
            auto Connection = Super::Bind<&T_DerivedSignal_Unreal::DoBroadcast>(UnrealMulticast, InHandle, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
        }
    }

    template <typename T_DerivedSignal, typename T_DerivedSignal_Unreal>
    auto
        TUtils_Signal_UnrealMulticast<T_DerivedSignal, T_DerivedSignal_Unreal>::
        Bind(
            FCk_Handle InHandle,
            UnrealDynamicDelegateType InDelegate,
            ECk_Signal_BindingPolicy InPayloadInFlightBehavior)
    {
        switch(InPayloadInFlightBehavior)
        {
            case ECk_Signal_BindingPolicy::FireIfPayloadInFlight:
                Bind<ECk_Signal_BindingPolicy::FireIfPayloadInFlight>(InHandle, InDelegate);
                break;
            case ECk_Signal_BindingPolicy::IgnorePayloadInFlight:
                Bind<ECk_Signal_BindingPolicy::IgnorePayloadInFlight>(InHandle, InDelegate);
                break;
            default:
                CK_INVALID_ENUM(InPayloadInFlightBehavior);
                break;
        }
    }

    template <typename T_DerivedSignal, typename T_DerivedSignal_Unreal>
    auto
        TUtils_Signal_UnrealMulticast<T_DerivedSignal, T_DerivedSignal_Unreal>::
        Unbind(
            FCk_Handle InHandle,
            UnrealDynamicDelegateType InDelegate)
    {
        if (NOT InHandle.Has<T_DerivedSignal_Unreal>())
        { return; }

        auto& UnrealMulticast = InHandle.AddOrGet<T_DerivedSignal_Unreal>();
        UnrealMulticast._Multicast.Remove(InDelegate);

        if (UnrealMulticast._Multicast.IsBound())
        { return; }

        UnrealMulticast._Connection.release();
        InHandle.Remove<T_DerivedSignal_Unreal>();
    }

    // --------------------------------------------------------------------------------------------------------------------
}
