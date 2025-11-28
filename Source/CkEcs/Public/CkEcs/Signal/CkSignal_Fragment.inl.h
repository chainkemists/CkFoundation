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

    template <typename T_DynamicDelegate, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    auto
        TFragment_Signal_Delegate<T_DynamicDelegate, T_PostFireBehavior, T_Args...>::ConditionalDynamicDelegate::
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

    template <typename T_DynamicDelegate, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    TFragment_Signal_Delegate<T_DynamicDelegate, T_PostFireBehavior, T_Args...>::
    ~TFragment_Signal_Delegate()
    {
        if (_Connection)
        { _Connection.release(); }
    }

    template <typename T_DynamicDelegate, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    auto
        TFragment_Signal_Delegate<T_DynamicDelegate, T_PostFireBehavior, T_Args...>::
        DoBroadcast(
            T_Args&&... InArgs)
        -> void
    {
        auto UnconditionalDelegatesCopy = _UnconditionalDelegates;
        auto ConditionalInvocationListCopy = _ConditionalInvocationList;

        for (const auto& Delegate : UnconditionalDelegatesCopy)
        {
            if (Delegate.IsBound())
            {
                Delegate.Execute(TTypeConverter<T_Args, TypeConverterPolicy::TypeToUnreal>{}(std::forward<T_Args>(InArgs))...);
            }
        }

        if (NOT ConditionalInvocationListCopy.IsEmpty())
        {
            for (const auto& ConditionalDynamicDelegate : ConditionalInvocationListCopy)
            {
                const auto& DelegateToInvoke = ConditionalDynamicDelegate.UnicastDelegate;
                const auto& ShouldInvokeFunc = ConditionalDynamicDelegate.InvocationPredicateFunc;

                if (ShouldInvokeFunc && NOT ShouldInvokeFunc(TTypeConverter<T_Args, ck::TypeConverterPolicy::TypeToUnreal>{}(std::forward<T_Args>(InArgs))...))
                { continue; }

                if (DelegateToInvoke.IsBound())
                {
                    DelegateToInvoke.Execute(TTypeConverter<T_Args, TypeConverterPolicy::TypeToUnreal>{}(std::forward<T_Args>(InArgs))...);
                }
            }
        }

        if constexpr (T_PostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
        {
            _UnconditionalDelegates.Empty();
            _ConditionalInvocationList.Empty();

            if (_Connection)
            { _Connection.release(); }
        }
    }

    template <typename T_DynamicDelegate, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    auto
        TFragment_Signal_Delegate<T_DynamicDelegate, T_PostFireBehavior, T_Args...>::
        DoGet_IsBound() const
        -> bool
    {
        return NOT _UnconditionalDelegates.IsEmpty() || NOT _ConditionalInvocationList.IsEmpty();
    }

    template <typename T_DynamicDelegate, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    auto
        TFragment_Signal_Delegate<T_DynamicDelegate, T_PostFireBehavior, T_Args...>::
        DoAddDelegate(
            DynamicDelegateType InDelegate,
            const DynamicDelegateInvocationPredicateFunc& InOptionalConditionalInvocationPredicate)
        -> void
    {
        if (InOptionalConditionalInvocationPredicate)
        {
            _ConditionalInvocationList.Add(ConditionalDynamicDelegate{InDelegate, InOptionalConditionalInvocationPredicate});
            return;
        }

        for (const auto& ExistingDelegate : _UnconditionalDelegates)
        {
            if (ExistingDelegate == InDelegate)
            { return; }
        }

        _UnconditionalDelegates.Add(InDelegate);
    }

    template <typename T_DynamicDelegate, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    auto
        TFragment_Signal_Delegate<T_DynamicDelegate, T_PostFireBehavior, T_Args...>::
        DoRemoveDelegate(
            DynamicDelegateType InDelegate)
        -> void
    {
        _ConditionalInvocationList.RemoveSingleSwap(ConditionalDynamicDelegate{InDelegate, nullptr});
        _UnconditionalDelegates.RemoveSingle(InDelegate);
    }

    // --------------------------------------------------------------------------------------------------------------------
}
