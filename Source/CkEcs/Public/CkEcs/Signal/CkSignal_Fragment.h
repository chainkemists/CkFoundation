#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/TypeConverter/CkTypeConverter.h"

#include "CkEcs/Tag/CkTag.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

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
        static_assert((std::is_reference_v<T_Args> || ...) == false, "None of the T_Args of a Signal can be references");
        static_assert((std::is_pointer_v<T_Args> || ...) == false, "None of the T_Args of a Signal can be raw pointers");

    public:
        CK_DEFINE_ECS_TAG(FTag_PayloadInFlight);

    public:
        using ArgsType = std::tuple<T_Args...>;
        using PayloadType = TOptional<ArgsType>;

        using ConnectionType = entt::connection;
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
        using DynamicDelegateType = typename MulticastType::FDelegate;
        using DynamicDelegateInvocationPredicateFunc = TFunction<bool(TTypeConverterReturnType<T_Args, TypeConverterPolicy::TypeToUnreal>...)>;

    public:
        struct ConditionalDynamicDelegate
        {
            DynamicDelegateType UnicastDelegate;
            DynamicDelegateInvocationPredicateFunc InvocationPredicateFunc;

            auto operator==(const ConditionalDynamicDelegate& InOther) const -> bool;
            CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ConditionalDynamicDelegate);
        };

        using DynamicDelegateInfoType = ConditionalDynamicDelegate;

    public:
        TFragment_Signal_UnrealMulticast() = default;
        TFragment_Signal_UnrealMulticast(const ThisType&) = delete;
        TFragment_Signal_UnrealMulticast(ThisType&& InOther) noexcept;
        ~TFragment_Signal_UnrealMulticast();

    public:
        auto operator=(ThisType InOther) -> ThisType& = delete;
        auto operator=(ThisType&& InOther) noexcept -> ThisType&;

    private:
        auto DoBroadcast(T_Args&&... InArgs) -> void;
        auto DoGet_IsBound() const -> bool;
        auto DoAddToMulticast(
            DynamicDelegateType InDelegate,
            const DynamicDelegateInvocationPredicateFunc& InOptionalConditionalInvocationPredicate) -> void;
        auto DoRemoveFromMulticast(
            DynamicDelegateType InDelegate) -> void;

    private:
        MulticastType _Multicast;
        TArray<ConditionalDynamicDelegate> _ConditionalInvocationList;
        ConnectionType _Connection;
        static constexpr auto PostFireBehavior = T_PostFireBehavior;

        CK_PROPERTY(_Connection);
        CK_PROPERTY(_Multicast);
        CK_PROPERTY(_ConditionalInvocationList);
    };
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkSignal_Fragment.inl.h"

// --------------------------------------------------------------------------------------------------------------------

