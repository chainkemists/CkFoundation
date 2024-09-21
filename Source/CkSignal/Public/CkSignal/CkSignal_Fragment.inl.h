#pragma once

#include "CkSignal_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    template <typename ... T_Args>
    TFragment_Signal<T_Args...>::
        TFragment_Signal()
        : _Invoke_Sink(_Invoke_Signal)
        , _InvokeAndUnbind_Sink(_InvokeAndUnbind_Signal)
    {
    }

    template <typename ... T_Args>
    TFragment_Signal<T_Args...>::
        TFragment_Signal(
            ThisType&& InOther) noexcept
        : _Payload(std::move(InOther._Payload))
        , _PayloadFrameNumber(std::move(InOther._PayloadFrameNumber))
        , _Invoke_Signal(std::move(InOther._Invoke_Signal))
        , _Invoke_Sink(std::move(InOther._Invoke_Sink))
        , _InvokeAndUnbind_Signal(std::move(InOther._InvokeAndUnbind_Signal))
        , _InvokeAndUnbind_Sink(std::move(InOther._InvokeAndUnbind_Sink))
    {
    }

    template <typename ... T_Args>
    auto
        TFragment_Signal<T_Args...>::
        operator==(
            ThisType&& InOther)
        -> ThisType&
    {
        return *this;
    }

    template <typename ... T_Args>
    auto
        TFragment_Signal<T_Args...>::
        Get_HasPayload() -> bool
    {
        return ck::IsValid(_Payload);
    }

    template <typename T_UnrealMulticast, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    auto
        TFragment_Signal_UnrealMulticast<T_UnrealMulticast, T_PostFireBehavior, T_Args...>::ConditionalDynamicDelegate::
        operator==(
            const ConditionalDynamicDelegate& InOther) const
        -> bool
    {
        return UnicastDelegate == InOther.UnicastDelegate;
    }

    // --------------------------------------------------------------------------------------------------------------------
}

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_UnrealMulticast, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    TFragment_Signal_UnrealMulticast<T_UnrealMulticast, T_PostFireBehavior, T_Args...>::
    TFragment_Signal_UnrealMulticast(
        ThisType&& InOther) noexcept
        : _Multicast(MoveTemp(InOther._Multicast))
        , _ConditionalInvocationList(MoveTemp(InOther._ConditionalInvocationList))
        , _Connection(MoveTemp(InOther._Connection))
    {
    }

    template <typename T_UnrealMulticast, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    auto
        TFragment_Signal_UnrealMulticast<T_UnrealMulticast, T_PostFireBehavior, T_Args...>::
        operator=(
            ThisType&& InOther) noexcept -> ThisType&
    {
        MoveTemp(_Multicast, InOther._Multicast);
        MoveTemp(_ConditionalInvocationList, InOther._ConditionalInvocationList);
        MoveTemp(_Connection, InOther._Connection);

        return *this;
    }

    template <typename T_UnrealMulticast, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    TFragment_Signal_UnrealMulticast<T_UnrealMulticast, T_PostFireBehavior, T_Args...>::
    ~TFragment_Signal_UnrealMulticast()
    {
        if (_Connection)
        { _Connection.release(); }
    }

    template <typename T_UnrealMulticast, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    auto
        TFragment_Signal_UnrealMulticast<T_UnrealMulticast, T_PostFireBehavior, T_Args...>::
        DoBroadcast(
            T_Args&&... InArgs)
        -> void
    {
        auto ConditionalInvocationListCopy = _ConditionalInvocationList;
        _Multicast.Broadcast(TTypeConverter<T_Args, TypeConverterPolicy::TypeToUnreal>{}(std::forward<T_Args>(InArgs))...);

        if (NOT ConditionalInvocationListCopy.IsEmpty())
        {
            auto ConditionalMulticast = MulticastType{};
            for (const auto& ConditionalDynamicDelegate : ConditionalInvocationListCopy)
            {
                const auto& DelegateToInvoke = ConditionalDynamicDelegate.UnicastDelegate;
                const auto& ShouldInvokeFunc = ConditionalDynamicDelegate.InvocationPredicateFunc;

                if (ShouldInvokeFunc && NOT ShouldInvokeFunc(TTypeConverter<T_Args, ck::TypeConverterPolicy::TypeToUnreal>{}(std::forward<T_Args>(InArgs))...))
                { continue; }

                ConditionalMulticast.Add(DelegateToInvoke);
            }

            ConditionalMulticast.Broadcast(TTypeConverter<T_Args, TypeConverterPolicy::TypeToUnreal>{}(std::forward<T_Args>(InArgs))...);
        }

        if constexpr (T_PostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
        {
            _Multicast.Clear();
            _ConditionalInvocationList.Empty();

            if (_Connection)
            { _Connection.release(); }
        }
    }

    template <typename T_UnrealMulticast, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    auto
        TFragment_Signal_UnrealMulticast<T_UnrealMulticast, T_PostFireBehavior, T_Args...>::
        DoGet_IsBound() const
        -> bool
    {
        return _Multicast.IsBound() || NOT _ConditionalInvocationList.IsEmpty();
    }

    template <typename T_UnrealMulticast, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    auto
        TFragment_Signal_UnrealMulticast<T_UnrealMulticast, T_PostFireBehavior, T_Args...>::
        DoAddToMulticast(
            DynamicDelegateType InDelegate,
            const DynamicDelegateInvocationPredicateFunc& InOptionalConditionalInvocationPredicate)
        -> void
    {
        if (InOptionalConditionalInvocationPredicate)
        {
            _ConditionalInvocationList.Add(ConditionalDynamicDelegate{InDelegate, InOptionalConditionalInvocationPredicate});
            return;
        }

        _Multicast.Add(InDelegate);
    }

    template <typename T_UnrealMulticast, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    auto
        TFragment_Signal_UnrealMulticast<T_UnrealMulticast, T_PostFireBehavior, T_Args...>::
        DoRemoveFromMulticast(
            DynamicDelegateType InDelegate)
        -> void
    {
        _ConditionalInvocationList.RemoveSingleSwap(ConditionalDynamicDelegate{InDelegate, nullptr});
        _Multicast.Remove(InDelegate);
    }

    // --------------------------------------------------------------------------------------------------------------------
}
