#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedSignal>
    class TUtils_Signal
    {
    public:
        using SignalType = T_DerivedSignal;
        using ConnectionType = typename SignalType::ConnectionType;

    public:
        template <typename T_HandleType>
        static auto Has(const T_HandleType& InHandle) -> bool;

        template <typename T_HandleType>
        static auto HasFiredAtLeastOnce(const T_HandleType& InHandle) -> bool;

        template <typename T_HandleType, typename... T_Args>
        static auto Broadcast(T_HandleType InHandle, TPayload<T_Args...>&& InPayload);

        template <auto T_Candidate, ECk_Signal_BindingPolicy InPayloadInFlightBehavior, ECk_Signal_PostFireBehavior InPostFireBehavior,
            typename T_HandleType>
        [[nodiscard]]
        static auto Bind(T_HandleType InHandle);

        template <auto T_Candidate,  ECk_Signal_BindingPolicy InPayloadInFlightBehavior, ECk_Signal_PostFireBehavior InPostFireBehavior,
            typename T_Instance, typename T_HandleType>
        [[nodiscard]]
        static auto Bind(T_Instance&& InInstance, T_HandleType InHandle);

        template <auto T_Candidate, typename T_HandleType>
        static auto Bind(T_HandleType InHandle, ECk_Signal_BindingPolicy InPayloadInFlightBehavior, ECk_Signal_PostFireBehavior InPostFireBehavior);

        template <auto T_Candidate, typename T_Instance, typename T_HandleType>
        [[nodiscard]]
        static auto Bind(T_Instance&& InInstance, T_HandleType InHandle, ECk_Signal_BindingPolicy InPayloadInFlightBehavior,
            ECk_Signal_PostFireBehavior InPostFireBehavior);

        template <auto T_Candidate, typename T_HandleType>
        static auto Unbind(T_HandleType InHandle);

        template <auto T_Candidate, typename T_Instance, typename T_HandleType>
        static auto Unbind(T_Instance&& InInstance, T_HandleType InHandle);
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedSignal, typename T_DerivedSignal_Delegate>
    class TUtils_Signal_Delegate : public TUtils_Signal<T_DerivedSignal>
    {
        CK_GENERATED_BODY(TUtils_Signal_Delegate<T_DerivedSignal COMMA T_DerivedSignal_Delegate>);

    public:
        using Super = TUtils_Signal<T_DerivedSignal>;
        using Super::SignalType;
        using Super::ConnectionType;

        using DelegateSignalType = T_DerivedSignal_Delegate;
        using DynamicDelegateType = typename T_DerivedSignal_Delegate::DynamicDelegateType;
        using ConditionalDynamicDelegateType = typename T_DerivedSignal_Delegate::ConditionalDynamicDelegate;
        using ConditionalDynamicDelegatePredicateFunc = typename T_DerivedSignal_Delegate::DynamicDelegateInvocationPredicateFunc;

    public:
        using Super::Bind;
        using Super::Unbind;

        template <ECk_Signal_BindingPolicy T_PayloadInFlightBehavior, typename T_HandleType>
        static auto
        Bind(
            T_HandleType InHandle,
            DynamicDelegateType InDelegate,
            ConditionalDynamicDelegatePredicateFunc InInvocationPredicate = nullptr);

        template <typename T_HandleType>
        static auto
        Bind(
            T_HandleType InHandle,
            DynamicDelegateType InDelegate,
            ECk_Signal_BindingPolicy InPayloadInFlightBehavior,
            ConditionalDynamicDelegatePredicateFunc InInvocationPredicate = nullptr);

        template <typename T_HandleType>
        static auto
        Unbind(
            T_HandleType InHandle,
            DynamicDelegateType InDelegate);

        template <typename T_HandleType>
        static auto
        IsBoundToDelegate(
            T_HandleType InHandle) -> bool;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkSignal_Utils.inl.h"

// --------------------------------------------------------------------------------------------------------------------
