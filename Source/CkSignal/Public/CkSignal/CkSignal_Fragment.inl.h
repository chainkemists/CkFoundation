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
    { }

    // --------------------------------------------------------------------------------------------------------------------
}

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

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
        DoBroadcastDelegate(
            auto InDelegate,
            T_Args&&... InArgs)
    {
        InDelegate.ExecuteIfBound(
            TTypeConverter<T_Args, ck::TypeConverterPolicy::TypeToUnreal>{}(std::forward<T_Args>(InArgs))...);
    }

    template <typename T_UnrealMulticast, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename ... T_Args>
    auto
        TFragment_Signal_UnrealMulticast<T_UnrealMulticast, T_PostFireBehavior, T_Args...>::
        DoBroadcast(
            T_Args&&... InArgs)
    {
        Get_Multicast().Broadcast(
            TTypeConverter<T_Args, ck::TypeConverterPolicy::TypeToUnreal>{}(std::forward<T_Args>(InArgs))...);

        //if constexpr (T_PostFireBehavior == ECk_Signal_PostFireBehavior::Unbind)
        //{
        //    _Multicast.Clear();

        //    if (_Connection)
        //    { _Connection.release(); }
        //}
    }

    // --------------------------------------------------------------------------------------------------------------------
}
