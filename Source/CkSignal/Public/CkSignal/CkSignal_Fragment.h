#pragma once

#include "CkCore/TypeConverter/CkTypeConverter.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename... T_Args>
    struct TFragment_Signal
    {
        template <typename>
        friend class TUtils_Signal;
        template <typename, typename>
        friend class TUtils_Signal_UnrealMulticast;

        CK_GENERATED_BODY(TFragment_Signal<T_Args...>);

        // ReSharper disable once CppInconsistentNaming
        static constexpr auto in_place_delete = true;

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
        TFragment_Signal(const ThisType&) = delete;
        TFragment_Signal(ThisType&& InOther) noexcept;

    public:
        auto operator==(ThisType) -> ThisType& = delete;
        auto operator==(ThisType&& InOther) -> ThisType&;

    private:
        PayloadType _Payload;
        int64 _PayloadFrameNumber = 0;

        SignalType _Invoke_Signal;
        SinkType _Invoke_Sink;

        SignalType _InvokeAndUnbind_Signal;
        SinkType _InvokeAndUnbind_Sink;

    private:
        CK_PROPERTY(_Payload);
        CK_PROPERTY(_Invoke_Signal);
        CK_PROPERTY(_Invoke_Sink);

        CK_PROPERTY(_InvokeAndUnbind_Signal);
        CK_PROPERTY(_InvokeAndUnbind_Sink);

    public:
        auto Get_HasPayload() -> bool;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_UnrealMulticast, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename... T_Args>
    struct TFragment_Signal_UnrealMulticast
    {
        // ReSharper disable once CppInconsistentNaming
        static constexpr auto in_place_delete = true;

        template <typename, typename>
        friend class TUtils_Signal_UnrealMulticast;

        CK_GENERATED_BODY(TFragment_Signal_UnrealMulticast<T_UnrealMulticast COMMA T_PostFireBehavior COMMA T_Args...>);

    public:
        using ConnectionType = entt::connection;
        using MulticastType = T_UnrealMulticast;

    public:
        TFragment_Signal_UnrealMulticast() = default;
        TFragment_Signal_UnrealMulticast(const ThisType&) = delete;
        TFragment_Signal_UnrealMulticast(ThisType&& InOther) noexcept;
        ~ TFragment_Signal_UnrealMulticast();

    public:
        auto operator=(ThisType InOther) -> ThisType& = delete;
        auto operator=(ThisType&& InOther) noexcept -> ThisType&;

    public:
        auto DoBroadcast(T_Args&&... InArgs);

    private:
        MulticastType _Multicast;
        ConnectionType _Connection;
        static constexpr auto PostFireBehavior = T_PostFireBehavior;

    private:
        CK_PROPERTY(_Connection);
        CK_PROPERTY(_Multicast);
    };
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkSignal_Fragment.inl.h"

// --------------------------------------------------------------------------------------------------------------------

