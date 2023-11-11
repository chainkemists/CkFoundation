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
    {
        Get_Multicast().Broadcast(
            TTypeConverter<T_Args, ck::TypeConverterPolicy::TypeToUnreal>{}(std::forward<T_Args>(InArgs))...);

        if constexpr (T_PostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
        {
            _Multicast.Clear();

            if (_Connection)
            { _Connection.release(); }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------
}
