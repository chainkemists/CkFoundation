#pragma once

#include "CkCore/Payload/CkPayload.h"

#include "CkSignal/CkSignal_Fragment_Data.h"
#include "CkSignal/CkSignal_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedSignal>
    class TUtils_Signal
    {
    public:
        using SignalType = T_DerivedSignal;

    public:
        static auto Has(FCk_Handle InHandle) -> bool;

        template <typename... T_Args>
        static auto Broadcast(FCk_Handle InHandle, TPayload<T_Args...>&& InPayload);

		template <auto T_Candidate, ECk_Signal_PayloadInFlight InPayloadInFlightBehavior>
		static auto Bind(FCk_Handle InHandle);

		template <auto T_Candidate,  ECk_Signal_PayloadInFlight InPayloadInFlightBehavior, typename T_Instance>
		static auto Bind(T_Instance&& InInstance, FCk_Handle InHandle);

		template <auto T_Candidate>
		static auto Bind(FCk_Handle InHandle, ECk_Signal_PayloadInFlight InPayloadInFlightBehavior);

		template <auto T_Candidate, typename T_Instance>
		static auto Bind(T_Instance&& InInstance, FCk_Handle InHandle, ECk_Signal_PayloadInFlight InPayloadInFlightBehavior);

		template <auto T_Candidate>
		static auto Unbind(FCk_Handle InHandle);

		template <auto T_Candidate, typename T_Instance>
		static auto Unbind(T_Instance&& InInstance, FCk_Handle InHandle);
    };

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
    template <auto T_Candidate, ECk_Signal_PayloadInFlight InPayloadInFlightBehavior>
    auto
    TUtils_Signal<T_DerivedSignal>::Bind(FCk_Handle InHandle)
    {
        const auto BroadcastIfPayloadInFlight = [&]<typename... T_Args>(TOptional<std::tuple<T_Args...>> InPayload)
        {
            if constexpr (InPayloadInFlightBehavior == ECk_Signal_PayloadInFlight::IgnorePayloadInFlight)
            { return; }

            if (ck::Is_NOT_Valid(InPayload))
            { return; }

            auto TempDelegate = SignalType::template DelegateType{};
            TempDelegate.template connect<T_Candidate>();
            std::apply(TempDelegate, InPayload);
        };

        auto& Signal = InHandle.AddOrGet<SignalType>();
        BroadcastIfPayloadInFlight(Signal._Payload);
        return Signal._Invoke_Sink.template connect<T_Candidate>();
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, ECk_Signal_PayloadInFlight InPayloadInFlightBehavior, typename T_Instance>
    auto
    TUtils_Signal<T_DerivedSignal>::Bind(T_Instance&& InInstance, FCk_Handle InHandle)
    {
        const auto BroadcastIfPayloadInFlight = [&]<typename... T_Args>(std::tuple<T_Args...> InPayload)
        {
            if (InPayloadInFlightBehavior == ECk_Signal_PayloadInFlight::IgnorePayloadInFlight)
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
    TUtils_Signal<T_DerivedSignal>::Bind(FCk_Handle InHandle,
        ECk_Signal_PayloadInFlight InPayloadInFlightBehavior)
    {
        switch(InPayloadInFlightBehavior)
        {
        case ECk_Signal_PayloadInFlight::FireIfPayloadInFlight:
            return Bind<T_Candidate, ECk_Signal_PayloadInFlight::FireIfPayloadInFlight>(InHandle);
        case ECk_Signal_PayloadInFlight::IgnorePayloadInFlight:
            return Bind<T_Candidate, ECk_Signal_PayloadInFlight::IgnorePayloadInFlight>(InHandle);
        default:
            // CK_INVALID_ENUM(InPayloadInFlightBehavior);
            break;
        }
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, typename T_Instance>
    auto
    TUtils_Signal<T_DerivedSignal>::Bind(T_Instance&& InInstance, FCk_Handle InHandle,
        ECk_Signal_PayloadInFlight InPayloadInFlightBehavior)
    {
        using ReturnType = decltype(Bind<T_Candidate, ECk_Signal_PayloadInFlight::FireIfPayloadInFlight>(std::forward<T_Instance>(InInstance), InHandle));

        switch(InPayloadInFlightBehavior)
        {
        case ECk_Signal_PayloadInFlight::FireIfPayloadInFlight:
            return Bind<T_Candidate, ECk_Signal_PayloadInFlight::FireIfPayloadInFlight>(std::forward<T_Instance>(InInstance), InHandle);
        case ECk_Signal_PayloadInFlight::IgnorePayloadInFlight:
            return Bind<T_Candidate, ECk_Signal_PayloadInFlight::IgnorePayloadInFlight>(std::forward<T_Instance>(InInstance), InHandle);
        default:
            // CK_INVALID_ENUM(InPayloadInFlightBehavior);
            break;
        }

        return ReturnType{};
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate>
    auto
    TUtils_Signal<T_DerivedSignal>::Unbind(FCk_Handle InHandle)
    {
        auto& Signal = InHandle.Get<SignalType>();
        Signal._Invoke_Sink.template disconnect<T_Candidate>();
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, typename T_Instance>
    auto
    TUtils_Signal<T_DerivedSignal>::Unbind(T_Instance&& InInstance, FCk_Handle InHandle)
    {
        auto& Signal = InHandle.Get<SignalType>();
        Signal._Invoke_Sink.template disconnect<T_Candidate>();
    }
}

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedSignal, typename T_DerivedSignal_Unreal>
    class TUtils_Signal_UnrealMulticast : public TUtils_Signal<T_DerivedSignal>
    {
        CK_GENERATED_BODY(TUtils_Signal_UnrealMulticast<T_DerivedSignal COMMA T_DerivedSignal_Unreal>);

    public:
        using Super = TUtils_Signal<T_DerivedSignal>;
        using Super::SignalType;

        using UnrealSignalType = T_DerivedSignal_Unreal;
        using UnrealDynamicDelegateType = typename T_DerivedSignal_Unreal::MulticastType::FDelegate;

    public:
        using Super::Bind;
        using Super::Unbind;

		template <ECk_Signal_PayloadInFlight T_PayloadInFlightBehavior>
        static auto Bind(FCk_Handle InHandle,
                         UnrealDynamicDelegateType InDelegate);

        static auto Bind(FCk_Handle InHandle, UnrealDynamicDelegateType InDelegate, ECk_Signal_PayloadInFlight InPayloadInFlightBehavior);

		static auto Unbind(FCk_Handle InHandle, UnrealDynamicDelegateType InDelegate);
    };

    template <typename T_DerivedSignal, typename T_DerivedSignal_Unreal>
    template <ECk_Signal_PayloadInFlight T_PayloadInFlightBehavior>
    auto
    TUtils_Signal_UnrealMulticast<T_DerivedSignal, T_DerivedSignal_Unreal>::Bind(FCk_Handle InHandle,
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
            auto Connection = Super::Bind<&T_DerivedSignal_Unreal::DoBroadcast>(UnrealMulticast, InHandle, ECk_Signal_PayloadInFlight::FireIfPayloadInFlight);
        }
    }

    template <typename T_DerivedSignal, typename T_DerivedSignal_Unreal>
    auto
    TUtils_Signal_UnrealMulticast<T_DerivedSignal, T_DerivedSignal_Unreal>::Bind(FCk_Handle InHandle,
        UnrealDynamicDelegateType InDelegate,
        ECk_Signal_PayloadInFlight InPayloadInFlightBehavior)
    {
        switch(InPayloadInFlightBehavior)
        {
        case ECk_Signal_PayloadInFlight::FireIfPayloadInFlight:
            Bind<ECk_Signal_PayloadInFlight::FireIfPayloadInFlight>(InHandle, InDelegate);
            break;
        case ECk_Signal_PayloadInFlight::IgnorePayloadInFlight:
            Bind<ECk_Signal_PayloadInFlight::IgnorePayloadInFlight>(InHandle, InDelegate);
            break;
        default:
            // CK_INVALID_ENUM(InPayloadInFlightBehavior);
            break;
        }
    }

    template <typename T_DerivedSignal, typename T_DerivedSignal_Unreal>
    auto
    TUtils_Signal_UnrealMulticast<T_DerivedSignal, T_DerivedSignal_Unreal>::Unbind(FCk_Handle InHandle,
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
