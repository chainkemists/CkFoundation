#pragma once

#include "CkSignal_Utils.h"

#include "CkCore/Time/CkTime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedSignal>
    template <typename T_HandleType>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Has(
            const T_HandleType& InHandle)
        -> bool
    {
        return InHandle.template Has<SignalType>();
    }

    template <typename T_DerivedSignal>
    template <typename T_HandleType>
    auto
        TUtils_Signal<T_DerivedSignal>::
        HasFiredAtLeastOnce(
            const T_HandleType& InHandle)
        -> bool
    {
        if (InHandle.template Has<SignalType>())
        {
            const auto& Signal  = InHandle.template Get<SignalType, ck::IsValid_Policy_IncludePendingKill>();
            return ck::IsValid(Signal._Payload);
        }

        return false;
    }

    template <typename T_DerivedSignal>
    template <typename T_HandleType, typename ... T_Args>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Broadcast(
            T_HandleType InHandle,
            TPayload<T_Args...>&& InPayload)
    {
        auto& Signal  = InHandle.template AddOrGet<SignalType, ck::IsValid_Policy_IncludePendingKill>();
        const auto Invoker = [&Signal](auto&&... InArgs)
        {
            Signal._Payload.Emplace(std::make_tuple(std::forward<T_Args>(InArgs)...));
            Signal._PayloadFrameNumber = UCk_Utils_Time_UE::Get_FrameCounter();

            Signal._Invoke_Signal.publish(InArgs...);

            Signal._InvokeAndUnbind_Signal.publish(InArgs...);
            Signal._InvokeAndUnbind_Sink.disconnect();
        };

        std::apply(Invoker, InPayload.Payload);

        InHandle.template AddOrGet<typename SignalType::FTag_PayloadInFlight, ck::IsValid_Policy_IncludePendingKill>();
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, ECk_Signal_BindingPolicy T_PayloadInFlightBehavior, ECk_Signal_PostFireBehavior InPostFireBehavior,
        typename T_HandleType>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Bind(
            T_HandleType InHandle)
    {
        const auto BroadcastIfPayloadInFlight = [&]<typename... T_Args>(std::tuple<T_Args...> InPayload) -> bool
        {
            if constexpr (T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::IgnorePayloadInFlight)
            {
                return false;
            }
            else
            {
                auto TempDelegate = SignalType::template DelegateType{};
                TempDelegate.template connect<T_Candidate>();
                std::apply(TempDelegate, InPayload);

                return true;
            }
        };

        auto& Signal = InHandle.template AddOrGet<SignalType>();
        using ReturnType = decltype(Signal._InvokeAndUnbind_Sink.template connect<T_Candidate>());

        if (ck::IsValid(Signal._Payload) && (Signal._PayloadFrameNumber == UCk_Utils_Time_UE::Get_FrameCounter() ||
            T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::FireIfPayloadInFlight))
        {
            // If the behavior is to Unbind, we do not need to 'connect' this candidate to the Signal
            if (BroadcastIfPayloadInFlight(*Signal._Payload) && InPostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
            { return ReturnType{}; }
        }

        if constexpr (InPostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
        { return Signal._InvokeAndUnbind_Sink.template connect<T_Candidate>(); }
        else
        { return Signal._Invoke_Sink.template connect<T_Candidate>(); }
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, ECk_Signal_BindingPolicy T_PayloadInFlightBehavior, ECk_Signal_PostFireBehavior InPostFireBehavior,
        typename T_Instance, typename T_HandleType>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Bind(
            T_Instance&& InInstance,
            T_HandleType InHandle)
    {
        const auto BroadcastIfPayloadInFlight = [&]<typename... T_Args>(std::tuple<T_Args...> InPayload) -> bool
        {
            if constexpr (T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::IgnorePayloadInFlight)
            {
                return false;
            }
            else
            {
                auto TempDelegate = SignalType::template DelegateType{};
                TempDelegate.template connect<T_Candidate>(InInstance);
                std::apply(TempDelegate, InPayload);

                return true;
            }
        };

        auto& Signal = InHandle.template AddOrGet<SignalType>();
        using ReturnType = decltype(Signal._InvokeAndUnbind_Sink.template connect<T_Candidate>(InInstance));

        if (ck::IsValid(Signal._Payload) && (Signal._PayloadFrameNumber == UCk_Utils_Time_UE::Get_FrameCounter() ||
            T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::FireIfPayloadInFlight))
        {
            // See notes in the other Bind function
            if (BroadcastIfPayloadInFlight(*Signal._Payload) && InPostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
            { return ReturnType{}; }
        }

        if constexpr (InPostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
        { return Signal._InvokeAndUnbind_Sink.template connect<T_Candidate>(InInstance); }
        else
        { return Signal._Invoke_Sink.template connect<T_Candidate>(InInstance); }
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, typename T_HandleType>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Bind(
            T_HandleType InHandle,
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
            {
                CK_INVALID_ENUM(InPayloadInFlightBehavior);
                break;
            }
        }

        return ReturnType{};
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, typename T_Instance, typename T_HandleType>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Bind(
            T_Instance&& InInstance,
            T_HandleType InHandle,
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
            {
                CK_INVALID_ENUM(InPayloadInFlightBehavior);
                break;
            }
        }

        return ReturnType{};
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, typename T_HandleType>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Unbind(
            T_HandleType InHandle)
    {
        auto& Signal = InHandle.template Get<SignalType, ck::IsValid_Policy_IncludePendingKill{}>();
        Signal._Invoke_Sink.template disconnect<T_Candidate>();
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, typename T_Instance, typename T_HandleType>
    auto
    TUtils_Signal<T_DerivedSignal>::
        Unbind(
            T_Instance&& InInstance,
            T_HandleType InHandle)
    {
        auto& Signal = InHandle.template Get<SignalType, ck::IsValid_Policy_IncludePendingKill{}>();
        Signal._Invoke_Sink.template disconnect<T_Candidate>(InInstance);
    }

    // --------------------------------------------------------------------------------------------------------------------
}

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedSignal, typename T_DerivedSignal_Delegate>
    template <ECk_Signal_BindingPolicy T_PayloadInFlightBehavior, typename T_HandleType>
    auto
        TUtils_Signal_Delegate<T_DerivedSignal, T_DerivedSignal_Delegate>::
        Bind(
            T_HandleType InHandle,
            DynamicDelegateType InDelegate,
            ConditionalDynamicDelegatePredicateFunc InInvocationPredicate)
    {
        if (NOT InDelegate.IsBound())
        { return; }

        auto& Signal          = InHandle.template AddOrGet<SignalType>();
        auto& DelegateFragment = InHandle.template AddOrGet<T_DerivedSignal_Delegate>();
        const auto& ContainsDelegateWithSignature = [&]()
        {
            if (const auto& IsConditionalDelegate = InInvocationPredicate != nullptr)
            { return false; }

            for (const auto& ExistingDelegate : DelegateFragment._UnconditionalDelegates)
            {
                if (ExistingDelegate == InDelegate)
                { return true; }
            }
            return false;
        }();

        CK_ENSURE_IF_NOT(NOT ContainsDelegateWithSignature,
            TEXT("The Delegate Signal already has the Delegate [{}] bound.[{}]"),
            InDelegate.GetFunctionName().ToString(),
            ck::Context(InHandle))
        { return; }

        // ReSharper disable once CppTooWideScope
        const auto EnsurePayloadInFlightIsOnlyFiredOnLatestDelegate = ck::IsValid(Signal.Get_Payload()) &&
            (T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::FireIfPayloadInFlight ||
            T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame);

        if (EnsurePayloadInFlightIsOnlyFiredOnLatestDelegate)
        {
            auto ExistingInvocationList = DelegateFragment._ConditionalInvocationList;
            auto ExistingUnconditionalDelegates = DelegateFragment._UnconditionalDelegates;

            DelegateFragment._ConditionalInvocationList.Empty();
            DelegateFragment._UnconditionalDelegates.Empty();
            DelegateFragment.DoAddDelegate(InDelegate, InInvocationPredicate);

            if (DelegateFragment._Connection)
            { DelegateFragment._Connection.release(); }

            auto Connection = Super::Bind <&T_DerivedSignal_Delegate::DoBroadcast>(
                DelegateFragment, InHandle, T_PayloadInFlightBehavior, T_DerivedSignal_Delegate::PostFireBehavior);

            if (Connection)
            {
                DelegateFragment._UnconditionalDelegates = ExistingUnconditionalDelegates;
                DelegateFragment._ConditionalInvocationList = ExistingInvocationList;
                DelegateFragment._Connection = Connection;
                DelegateFragment.DoAddDelegate(InDelegate, InInvocationPredicate);

                const auto& IsBound = DelegateFragment.DoGet_IsBound();

                CK_ENSURE((DelegateFragment._Connection && IsBound) ||
                    (NOT DelegateFragment._Connection && NOT IsBound),
                    TEXT("Expected Connection to be VALID if delegates bound OR INVALID "
                         "if no delegates bound on Signal [{}] with Delegate Signal [{}] on Entity [{}] with BindingPolicy is [{}]. "
                         "This ensure hints to a logical problem somewhere in the Signals logic (or this Ensure)."),
                    ck::Get_RuntimeTypeToString<T_DerivedSignal>(),
                    ck::Get_RuntimeTypeToString<T_DerivedSignal_Delegate>(),
                    InHandle,
                    T_PayloadInFlightBehavior);
            }
        }
        else
        {
            DelegateFragment.DoAddDelegate(InDelegate, InInvocationPredicate);

            if (DelegateFragment._Connection)
            { DelegateFragment._Connection.release(); }

            auto Connection = Super::Bind <&T_DerivedSignal_Delegate::DoBroadcast>(
                DelegateFragment, InHandle, T_PayloadInFlightBehavior, T_DerivedSignal_Delegate::PostFireBehavior);
            DelegateFragment._Connection = Connection;
        }
    }

    template <typename T_DerivedSignal, typename T_DerivedSignal_Delegate>
    template <typename T_HandleType>
    auto
        TUtils_Signal_Delegate<T_DerivedSignal, T_DerivedSignal_Delegate>::
        Bind(
            T_HandleType InHandle,
            DynamicDelegateType InDelegate,
            ECk_Signal_BindingPolicy InPayloadInFlightBehavior,
            ConditionalDynamicDelegatePredicateFunc InInvocationPredicate)
    {
        if (NOT InDelegate.IsBound())
        { return; }

        switch(InPayloadInFlightBehavior)
        {
            case ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame:
            {
                Bind<ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame>(InHandle, InDelegate, InInvocationPredicate);
                break;
            }
            case ECk_Signal_BindingPolicy::FireIfPayloadInFlight:
            {
                Bind<ECk_Signal_BindingPolicy::FireIfPayloadInFlight>(InHandle, InDelegate, InInvocationPredicate);
                break;
            }
            case ECk_Signal_BindingPolicy::IgnorePayloadInFlight:
            {
                Bind<ECk_Signal_BindingPolicy::IgnorePayloadInFlight>(InHandle, InDelegate, InInvocationPredicate);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(InPayloadInFlightBehavior);
                break;
            }
        }
    }

    template <typename T_DerivedSignal, typename T_DerivedSignal_Delegate>
    template <typename T_HandleType>
    auto
        TUtils_Signal_Delegate<T_DerivedSignal, T_DerivedSignal_Delegate>::
        Unbind(
            T_HandleType InHandle,
            DynamicDelegateType InDelegate)
    {
        if (NOT InDelegate.IsBound())
        { return; }

        if (NOT InHandle.template Has<T_DerivedSignal_Delegate>())
        { return; }

        auto& DelegateFragment = InHandle.template Get<T_DerivedSignal_Delegate, ck::IsValid_Policy_IncludePendingKill>();
        DelegateFragment.DoRemoveDelegate(InDelegate);

        if (DelegateFragment.DoGet_IsBound())
        { return; }

        DelegateFragment._Connection.release();
        InHandle.template Remove<T_DerivedSignal_Delegate>();
    }

    template <typename T_DerivedSignal, typename T_DerivedSignal_Delegate>
    template <typename T_HandleType>
    auto
        TUtils_Signal_Delegate<T_DerivedSignal, T_DerivedSignal_Delegate>::
        IsBoundToDelegate(
            T_HandleType InHandle)
        -> bool
    {
        if (NOT InHandle.template Has<T_DerivedSignal_Delegate>())
        { return {}; }

        const auto& DelegateFragment = InHandle.template Get<T_DerivedSignal_Delegate, ck::IsValid_Policy_IncludePendingKill>();

        return DelegateFragment.DoGet_IsBound();
    }

    // --------------------------------------------------------------------------------------------------------------------
}
