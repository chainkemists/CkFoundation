#pragma once

#include "CkCore/TypeConverter/CkTypeConverter.h"

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedSignal, typename... T_Args>
    struct TFragment_Signal
    {
        template <typename>
        friend class TUtils_Signal;
        template <typename, typename>
        friend class TUtils_Signal_UnrealMulticast;

        CK_GENERATED_BODY(TFragment_Signal<T_DerivedSignal COMMA T_Args...>);

    public:
        struct FTag_PayloadInFlight {};

    public:
        using ArgsType = std::tuple<T_Args...>;
        using PayloadType = TOptional<ArgsType>;

        using SignalType = entt::sigh<void(T_Args...)>;
        using SinkType = entt::sink<SignalType>;
        using DelegateType = entt::delegate<void(T_Args...)>;

    public:
        TFragment_Signal();

    private:
        PayloadType _Payload;
        SignalType _Invoke_Signal;
        SinkType _Invoke_Sink;

    private:
        CK_PROPERTY(_Payload);
        CK_PROPERTY(_Invoke_Signal);
        CK_PROPERTY(_Invoke_Sink);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedSignal, typename T_UnrealMulticast, typename... T_Args>
    struct TFragment_Signal_UnrealMulticast
    {
        template <typename, typename>
        friend class TUtils_Signal_UnrealMulticast;

        CK_GENERATED_BODY(TFragment_Signal_UnrealMulticast<T_DerivedSignal COMMA T_UnrealMulticast COMMA T_Args...>);

    public:
        using ConnectionType = entt::connection;
        using MulticastType = T_UnrealMulticast;

    public:
        auto DoBroadcast(T_Args&&... InArgs);

    private:
        MulticastType _Multicast;
        ConnectionType _Connection;

    private:
        CK_PROPERTY(_Multicast);
        CK_PROPERTY(_Connection);
    };
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkSignal_Fragment.inl.h"

// --------------------------------------------------------------------------------------------------------------------

