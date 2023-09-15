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
    { }

    // --------------------------------------------------------------------------------------------------------------------
}

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_UnrealMulticast, typename ... T_Args>
    auto
        TFragment_Signal_UnrealMulticast<T_UnrealMulticast, T_Args...>::
        DoBroadcast(
            T_Args&&... InArgs)
    {
        Get_Multicast().Broadcast(
            TTypeConverter<T_Args, ck::TypeConverterPolicy::TypeToUnreal>{}(std::forward<T_Args>(InArgs))...);
    }

    // --------------------------------------------------------------------------------------------------------------------
}
