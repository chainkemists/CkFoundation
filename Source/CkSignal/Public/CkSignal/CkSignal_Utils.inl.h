#pragma once

#include "CkSignal_Utils.inl.h"

#include "CkCore/Time/CkTime_Utils.h"

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
        auto& Signal = InHandle.AddOrGet<SignalType>();
        const auto Invoker = [&Signal](auto&&... InArgs)
        {
            Signal._Invoke_Signal.publish(InArgs...);

            Signal._InvokeAndUnbind_Signal.publish(InArgs...);
            Signal._InvokeAndUnbind_Sink.disconnect();

            Signal._Payload.Emplace(std::make_tuple(std::forward<T_Args>(InArgs)...));
            Signal._PayloadFrameNumber = UCk_Utils_Time_UE::Get_FrameCounter();
        };

        std::apply(Invoker, InPayload.Payload);

        InHandle.AddOrGet<typename SignalType::FTag_PayloadInFlight>();
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, ECk_Signal_BindingPolicy T_PayloadInFlightBehavior, ECk_Signal_PostFireBehavior InPostFireBehavior>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Bind(
            FCk_Handle InHandle)
    {
        const auto BroadcastIfPayloadInFlight = [&]<typename... T_Args>(std::tuple<T_Args...> InPayload) -> bool
        {
            if constexpr (T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::IgnorePayloadInFlight)
            { return false; }

            auto TempDelegate = SignalType::template DelegateType{};
            TempDelegate.template connect<T_Candidate>();
            std::apply(TempDelegate, InPayload);

            return true;
        };

        auto& Signal = InHandle.AddOrGet<SignalType>();
        using ReturnType = decltype(Signal._InvokeAndUnbind_Sink.template connect<T_Candidate>());

        if (ck::IsValid(Signal._Payload) && (Signal._PayloadFrameNumber == GFrameCounter ||
            T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::FireIfPayloadInFlight))
        {
            // If the behavior is to Unbind, we do not need to 'connect' this candidate to the Signal
            if (BroadcastIfPayloadInFlight(*Signal._Payload) && InPostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
            { return ReturnType{}; }
        }

        if constexpr (InPostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
        { return Signal._InvokeAndUnbind_Sink.template connect<T_Candidate>(); }

        return Signal._Invoke_Sink.template connect<T_Candidate>();
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, ECk_Signal_BindingPolicy T_PayloadInFlightBehavior, ECk_Signal_PostFireBehavior InPostFireBehavior,
        typename T_Instance>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Bind(
            T_Instance&& InInstance,
            FCk_Handle InHandle)
    {
        const auto BroadcastIfPayloadInFlight = [&]<typename... T_Args>(std::tuple<T_Args...> InPayload) -> bool
        {
            if (T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::IgnorePayloadInFlight)
            { return false; }


            auto TempDelegate = SignalType::template DelegateType{};
            TempDelegate.template connect<T_Candidate>(InInstance);
            std::apply(TempDelegate, InPayload);

            return true;
        };

        auto& Signal = InHandle.AddOrGet<SignalType>();
        using ReturnType = decltype(Signal._InvokeAndUnbind_Sink.template connect<T_Candidate>(InInstance));

        if (ck::IsValid(Signal._Payload) && (Signal._PayloadFrameNumber == GFrameCounter ||
            T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::FireIfPayloadInFlight))
        {
            // See notes in the other Bind function
            if (BroadcastIfPayloadInFlight(*Signal._Payload) && InPostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
            { return ReturnType{}; }
        }

        if constexpr (InPostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
        { return Signal._InvokeAndUnbind_Sink.template connect<T_Candidate>(InInstance); }

        return Signal._Invoke_Sink.template connect<T_Candidate>(InInstance);
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Bind(
            FCk_Handle InHandle,
            ECk_Signal_BindingPolicy InPayloadInFlightBehavior,
            ECk_Signal_PostFireBehavior InPostFireBehavior)
    {
        using ReturnType = decltype(Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight, ECk_Signal_PostFireBehavior::DoNothing>(InHandle));

        switch(InPayloadInFlightBehavior)
        {
            case ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame:
            {
                if (InPostFireBehavior == ECk_Signal_PostFireBehavior::DoNothing)
                { return Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame, ECk_Signal_PostFireBehavior::DoNothing>(InHandle); }

                return Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame, ECk_Signal_PostFireBehavior::Unbind>(InHandle);
            }
            case ECk_Signal_BindingPolicy::FireIfPayloadInFlight:
            {
                if (InPostFireBehavior == ECk_Signal_PostFireBehavior::DoNothing)
                { return Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight, ECk_Signal_PostFireBehavior::DoNothing>(InHandle); }

                return Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight, ECk_Signal_PostFireBehavior::Unbind>(InHandle);
            }
            case ECk_Signal_BindingPolicy::IgnorePayloadInFlight:
            {
                if (InPostFireBehavior == ECk_Signal_PostFireBehavior::DoNothing)
                { return Bind<T_Candidate, ECk_Signal_BindingPolicy::IgnorePayloadInFlight, ECk_Signal_PostFireBehavior::DoNothing>(InHandle); }

                return Bind<T_Candidate, ECk_Signal_BindingPolicy::IgnorePayloadInFlight, ECk_Signal_PostFireBehavior::Unbind>(InHandle);
            }
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
            ECk_Signal_BindingPolicy InPayloadInFlightBehavior,
            ECk_Signal_PostFireBehavior InPostFireBehavior)
    {
        using ReturnType = decltype(Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight, ECk_Signal_PostFireBehavior::DoNothing>(
            std::forward<T_Instance>(InInstance), InHandle));

        switch(InPayloadInFlightBehavior)
        {
            case ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame:
            {
                if (InPostFireBehavior == ECk_Signal_PostFireBehavior::DoNothing)
                {
                    return Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame, ECk_Signal_PostFireBehavior::DoNothing>(
                        std::forward<T_Instance>(InInstance), InHandle);
                }

                return Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame, ECk_Signal_PostFireBehavior::Unbind>(
                    std::forward<T_Instance>(InInstance), InHandle);
            }
            case ECk_Signal_BindingPolicy::FireIfPayloadInFlight:
            {
                if (InPostFireBehavior == ECk_Signal_PostFireBehavior::DoNothing)
                {
                    return Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight, ECk_Signal_PostFireBehavior::DoNothing>(
                        std::forward<T_Instance>(InInstance), InHandle);
                }

                return Bind<T_Candidate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight, ECk_Signal_PostFireBehavior::Unbind>(
                    std::forward<T_Instance>(InInstance), InHandle);
            }
            case ECk_Signal_BindingPolicy::IgnorePayloadInFlight:
            {
                if (InPostFireBehavior == ECk_Signal_PostFireBehavior::DoNothing)
                {
                    return Bind<T_Candidate, ECk_Signal_BindingPolicy::IgnorePayloadInFlight, ECk_Signal_PostFireBehavior::DoNothing>(
                        std::forward<T_Instance>(InInstance), InHandle);
                }

                return Bind<T_Candidate, ECk_Signal_BindingPolicy::IgnorePayloadInFlight, ECk_Signal_PostFireBehavior::Unbind>(
                    std::forward<T_Instance>(InInstance), InHandle);
            }
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
                         TEXT("The Unreal Multicast already has the Delegate [{}] bound.[{}]"),
            InDelegate.GetFunctionName().ToString(),
            ck::Context(InHandle))
        { return; }

        //const auto BroadcastIfPayloadInFlight = [&]<typename... T_Args>(std::tuple<T_Args...> InPayload) -> bool
        //{
        //    if constexpr (T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::IgnorePayloadInFlight)
        //    { return false; }

        //    std::apply(std::bind_front(&T_DerivedSignal_Unreal::DoBroadcastDelegate, &UnrealMulticast), std::make_tuple(InDelegate, InPayload));

        //    return true;
        //};

        //if (ck::IsValid(Signal._Payload) && (Signal._PayloadFrameNumber == GFrameCounter ||
        //    T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::FireIfPayloadInFlight))
        //{
        //    // If the behavior is to Unbind, we do not need to 'connect' this candidate to the Signal
        //    if (BroadcastIfPayloadInFlight(*Signal._Payload) && T_DerivedSignal_Unreal::PostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
        //    { return; }
        //}

        auto TempMulticast = UnrealMulticast._Multicast;

        UnrealMulticast._Multicast.Clear();
        UnrealMulticast._Multicast.AddUnique(InDelegate);

        if (UnrealMulticast._Connection)
        { UnrealMulticast._Connection.release(); }

        auto Connection = Super::Bind <&T_DerivedSignal_Unreal::DoBroadcast>(
            UnrealMulticast, InHandle, T_PayloadInFlightBehavior, T_DerivedSignal_Unreal::PostFireBehavior);

        //CK_ENSURE_IF_NOT(((UnrealMulticast._Multicast.IsBound() && Connection) ||
        //    (NOT UnrealMulticast._Multicast.IsBound() && NOT Connection)),
        //    TEXT("Our assumption that when we receive a valid Connection that we also have Bound multicasts is INVALID. "
        //        "We may receive an invalid Connection IFF the above previous Bind broadcasts payloads in flight AND the post-fire "
        //        "behavior was to unbind; in which case, the Multicast is cleared (see DoBroadcast). The assumption of this "
        //        "ensure check must hold true. If not, either this assumption is incorrect OR this is a symptom of a larger "
        //        "problem.\n\nContext: Entity [{}] trying to bind with [{}]"),
        //    InHandle,
        //    InDelegate.GetFunctionName().ToString())
        //{ return; }

        UnrealMulticast._Multicast = TempMulticast;
        UnrealMulticast._Multicast.Add(InDelegate);
        UnrealMulticast._Connection = Connection;
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
            case ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame:
            {
                Bind<ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame>(InHandle, InDelegate);
                break;
            }
            case ECk_Signal_BindingPolicy::FireIfPayloadInFlight:
            {
                Bind<ECk_Signal_BindingPolicy::FireIfPayloadInFlight>(InHandle, InDelegate);
                break;
            }
            case ECk_Signal_BindingPolicy::IgnorePayloadInFlight:
            {
                Bind<ECk_Signal_BindingPolicy::IgnorePayloadInFlight>(InHandle, InDelegate);
                break;
            }
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
