#pragma once

#include "CkSignal_Utils.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Time/CkTime_Utils.h"
#include "CkCore/Validation/CkUntracedStructSafety.h"

#include <StructUtils/InstancedStruct.h>

#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_signal_utils
{
    template <typename T_SignalType, typename T_HandleType, typename T_ValidityPolicy>
    auto
        ValidateSignalHandle(
            const T_HandleType& InHandle,
            const TCHAR* InOperation,
            T_ValidityPolicy InValidityPolicy)
        -> bool
    {
        const auto IsHandleValid = ck::IsValid(InHandle, InValidityPolicy);
        CK_ENSURE_IF_NOT(IsHandleValid,
            TEXT("Signal [{}] operation [{}] rejected invalid/tombstone handle"),
            ck::Get_RuntimeTypeToString<T_SignalType>(),
            InOperation)
        { return false; }

        return true;
    }

    inline auto
        ValidateRetainedInstancedStruct(
            const FInstancedStruct& InPayload,
            const FString& InSignalType)
        -> bool
    {
        if (NOT InPayload.IsValid())
        { return true; }

        const auto* PayloadStruct = InPayload.GetScriptStruct();
        const auto HasPayloadStruct = PayloadStruct != nullptr;
        CK_ENSURE_IF_NOT(HasPayloadStruct,
            TEXT("Signal [{}] rejected an InstancedStruct payload without a reflected struct type"),
            InSignalType)
        { return false; }

        const auto Safety = ck::Analyze_UntracedStructSafety(PayloadStruct);
        const auto IsPayloadSafe = Safety.IsGcIndependent();
        CK_ENSURE_IF_NOT(IsPayloadSafe,
            TEXT("Signal [{}] rejected unsafe InstancedStruct payload [{}]; [{}]: {}"),
            InSignalType,
            PayloadStruct->GetName(),
            Safety.FailurePath,
            Safety.FailureReason)
        { return false; }

        return true;
    }

    inline auto
        ValidateRetainedPayloadArg(
            const FInstancedStruct& InArg,
            const FString& InSignalType)
        -> bool
    { return ValidateRetainedInstancedStruct(InArg, InSignalType); }

    template <typename T_Arg>
    auto
        ValidateRetainedPayloadArg(
            const T_Arg&,
            const FString&)
        -> bool
    { return true; }

    template <typename... T_Args>
    auto
        ValidateRetainedPayload(
            const ck::TPayload<T_Args...>& InPayload,
            const FString& InSignalType)
        -> bool
    {
        return std::apply([&](const auto&... InArgs) -> bool
        {
            return (true && ... && ValidateRetainedPayloadArg(InArgs, InSignalType));
        }, InPayload.Payload);
    }
}

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
        if (ck::Is_NOT_Valid(InHandle, ck::IsValid_Policy_IncludePendingKill{}))
        { return false; }

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
        if (ck::Is_NOT_Valid(InHandle, ck::IsValid_Policy_IncludePendingKill{}))
        { return false; }

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
        if (NOT ck_signal_utils::ValidateSignalHandle<SignalType>(
                InHandle, TEXT("Broadcast"), ck::IsValid_Policy_IncludePendingKill{}))
        { return; }

        if constexpr ((false || ... || std::is_same_v<std::remove_cvref_t<T_Args>, FInstancedStruct>))
        {
            const auto SignalTypeName = ck::Get_RuntimeTypeToString<SignalType>();
            if (NOT ck_signal_utils::ValidateRetainedPayload(InPayload, SignalTypeName))
            { return; }
        }

        auto& Signal  = InHandle.template AddOrGet<SignalType, ck::IsValid_Policy_IncludePendingKill>();

        CK_STAT(STAT_SignalBroadcast);

        const auto Invoker = [&Signal](auto&&... InArgs)
        {
            // Store the payload tuple FIRST: forward<T_Args>() moves value-type args, so InArgs may be
            // moved-from after this line. publish() below must therefore source from the STORED tuple —
            // sourcing from InArgs delivers moved-from values (empty TArrays inside USTRUCT payloads).
            Signal._Payload.Emplace(std::make_tuple(std::forward<T_Args>(InArgs)...));
            Signal._PayloadFrameNumber = UCk_Utils_Time_UE::Get_FrameCounter();

            std::apply([&](auto&... StoredArgs)
            {
                Signal._Invoke_Signal.publish(StoredArgs...);
                Signal._InvokeAndUnbind_Signal.publish(StoredArgs...);
            }, *Signal._Payload);

            Signal._InvokeAndUnbind_Sink.disconnect();
        };

        std::apply(Invoker, InPayload.Payload);
    }

    template <typename T_DerivedSignal>
    template <auto T_Candidate, ECk_Signal_BindingPolicy T_PayloadInFlightBehavior, ECk_Signal_PostFireBehavior InPostFireBehavior,
        typename T_HandleType>
    auto
        TUtils_Signal<T_DerivedSignal>::
        Bind(
            T_HandleType InHandle)
    {
        using ReturnType = typename SignalType::ConnectionType;
        if (NOT ck_signal_utils::ValidateSignalHandle<SignalType>(
                InHandle, TEXT("Bind"), ck::IsValid_Policy_Default{}))
        { return ReturnType{}; }

        auto& Signal = InHandle.template AddOrGet<SignalType>();

        const auto ShouldFirePayloadInFlight = ck::IsValid(Signal._Payload) &&
            (Signal._PayloadFrameNumber == UCk_Utils_Time_UE::Get_FrameCounter() ||
            T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::FireIfPayloadInFlight);

        if (ShouldFirePayloadInFlight)
        {
            if constexpr (T_PayloadInFlightBehavior != ECk_Signal_BindingPolicy::IgnorePayloadInFlight)
            {
                auto TempDelegate = typename SignalType::DelegateType{};
                TempDelegate.template connect<T_Candidate>();
                std::apply(TempDelegate, *Signal._Payload);

                if constexpr (InPostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
                { return ReturnType{}; }
            }
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
        using ReturnType = typename SignalType::ConnectionType;
        if (NOT ck_signal_utils::ValidateSignalHandle<SignalType>(
                InHandle, TEXT("Bind"), ck::IsValid_Policy_Default{}))
        { return ReturnType{}; }

        auto& Signal = InHandle.template AddOrGet<SignalType>();

        const auto ShouldFirePayloadInFlight = ck::IsValid(Signal._Payload) &&
            (Signal._PayloadFrameNumber == UCk_Utils_Time_UE::Get_FrameCounter() ||
            T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::FireIfPayloadInFlight);

        if (ShouldFirePayloadInFlight)
        {
            if constexpr (T_PayloadInFlightBehavior != ECk_Signal_BindingPolicy::IgnorePayloadInFlight)
            {
                auto TempDelegate = typename SignalType::DelegateType{};
                TempDelegate.template connect<T_Candidate>(InInstance);
                std::apply(TempDelegate, *Signal._Payload);

                if constexpr (InPostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
                { return ReturnType{}; }
            }
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
        if (NOT ck_signal_utils::ValidateSignalHandle<SignalType>(
                InHandle, TEXT("Bind"), ck::IsValid_Policy_Default{}))
        { return ReturnType{}; }

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
        if (NOT ck_signal_utils::ValidateSignalHandle<SignalType>(
                InHandle, TEXT("Bind"), ck::IsValid_Policy_Default{}))
        { return ReturnType{}; }

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
        if (NOT ck_signal_utils::ValidateSignalHandle<SignalType>(
                InHandle, TEXT("Unbind"), ck::IsValid_Policy_IncludePendingKill{}))
        { return; }

        const auto HasSignalState = InHandle.template Has<SignalType>();
        CK_ENSURE_IF_NOT(HasSignalState,
            TEXT("Signal [{}] Unbind rejected a handle without signal state"),
            ck::Get_RuntimeTypeToString<SignalType>())
        { return; }

        auto& Signal = InHandle.template Get<SignalType, ck::IsValid_Policy_IncludePendingKill>();
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
        if (NOT ck_signal_utils::ValidateSignalHandle<SignalType>(
                InHandle, TEXT("Unbind"), ck::IsValid_Policy_IncludePendingKill{}))
        { return; }

        const auto HasSignalState = InHandle.template Has<SignalType>();
        CK_ENSURE_IF_NOT(HasSignalState,
            TEXT("Signal [{}] Unbind rejected a handle without signal state"),
            ck::Get_RuntimeTypeToString<SignalType>())
        { return; }

        auto& Signal = InHandle.template Get<SignalType, ck::IsValid_Policy_IncludePendingKill>();
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
        if (NOT ck_signal_utils::ValidateSignalHandle<SignalType>(
                InHandle, TEXT("Delegate Bind"), ck::IsValid_Policy_Default{}))
        { return; }

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

        // A parked pool instance keeps its pointer identity, so a bind it made on an entity that
        // outlives it would leak into its next vend. Hook the pool's release to disconnect;
        // quiet no-op for untracked bind targets.
        if (auto* BoundObject = InDelegate.GetUObject();
            ck::IsValid(BoundObject))
        {
            UCk_Utils_Object_UE::TryRegisterPoolReleaseQuiesceHook(BoundObject,
                [Handle = InHandle, Delegate = InDelegate]()
                {
                    if (ck::Is_NOT_Valid(Handle))
                    { return; }

                    Unbind(Handle, Delegate);
                });
        }

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

            auto Connection = Super::template Bind<&T_DerivedSignal_Delegate::DoBroadcast>(
                DelegateFragment, InHandle, T_PayloadInFlightBehavior, T_DerivedSignal_Delegate::PostFireBehavior);

            if (ck::Is_NOT_Valid(InHandle) || NOT InHandle.template Has<T_DerivedSignal_Delegate>())
            { return; }

            if (Connection)
            {
                auto& DelegateFragmentPostBroadcast = InHandle.template Get<T_DerivedSignal_Delegate>();
                DelegateFragmentPostBroadcast._UnconditionalDelegates = ExistingUnconditionalDelegates;
                DelegateFragmentPostBroadcast._ConditionalInvocationList = ExistingInvocationList;
                DelegateFragmentPostBroadcast._Connection = Connection;
                DelegateFragmentPostBroadcast.DoAddDelegate(InDelegate, InInvocationPredicate);

                const auto& IsBound = DelegateFragmentPostBroadcast.DoGet_IsBound();

                CK_ENSURE((DelegateFragmentPostBroadcast._Connection && IsBound) ||
                    (NOT DelegateFragmentPostBroadcast._Connection && NOT IsBound),
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

            auto Connection = Super::template Bind<&T_DerivedSignal_Delegate::DoBroadcast>(
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
        if (NOT ck_signal_utils::ValidateSignalHandle<SignalType>(
                InHandle, TEXT("Delegate Bind"), ck::IsValid_Policy_Default{}))
        { return; }

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
        if (NOT ck_signal_utils::ValidateSignalHandle<SignalType>(
                InHandle, TEXT("Delegate Unbind"), ck::IsValid_Policy_IncludePendingKill{}))
        { return; }

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
        if (ck::Is_NOT_Valid(InHandle, ck::IsValid_Policy_IncludePendingKill{}))
        { return false; }

        if (NOT InHandle.template Has<T_DerivedSignal_Delegate>())
        { return {}; }

        const auto& DelegateFragment = InHandle.template Get<T_DerivedSignal_Delegate, ck::IsValid_Policy_IncludePendingKill>();

        return DelegateFragment.DoGet_IsBound();
    }

    // --------------------------------------------------------------------------------------------------------------------
}
