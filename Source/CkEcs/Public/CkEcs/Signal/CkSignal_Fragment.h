#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/TypeConverter/CkTypeConverter.h"

#include "CkEcs/Tag/CkTag.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkProfile/Stats/CkStats.h"

#include <entt/signal/sigh.hpp>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("CkSignals_Listeners"), STATGROUP_CkSignals_Listeners, STATCAT_Advanced);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Per-LISTENER signal profiling, opt-in via `ck.Signal.StatListeners 1` — the dynamic stat-id
    // construction is not free, so it stays off the hot path by default.
    CKECS_API auto Get_ShouldStat_SignalListeners() -> bool;
    CKECS_API auto Make_SignalListenerStatId(const FString& InListenerName) -> TStatId;

    // --------------------------------------------------------------------------------------------------------------------

    template <typename... T_Args>
    struct TFragment_Signal
    {
        template <typename>
        friend class TUtils_Signal;
        template <typename, typename>
        friend class TUtils_Signal_Delegate;

        CK_GENERATED_BODY(TFragment_Signal<T_Args...>);

        // ReSharper disable once CppInconsistentNaming
        static constexpr auto in_place_delete = true;

    public:
        static_assert((std::is_reference_v<T_Args> || ...) == false, "None of the T_Args of a Signal can be references");
        static_assert((std::is_pointer_v<T_Args> || ...) == false, "None of the T_Args of a Signal can be raw pointers");

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
        auto operator=(const ThisType&) -> ThisType& = delete;
        auto operator=(ThisType&& InOther) noexcept -> ThisType&;

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

    template <typename T_DynamicDelegate, ECk_Signal_PostFireBehavior T_PostFireBehavior, typename... T_Args>
    struct TFragment_Signal_Delegate
    {
        // ReSharper disable once CppInconsistentNaming
        static constexpr auto in_place_delete = true;

        template <typename, typename>
        friend class TUtils_Signal_Delegate;

#if WITH_DEV_AUTOMATION_TESTS
        // Lets the teardown UAF repro spec aim a REAL fragment at a sigh the spec owns and can
        // poison, proving both halves of the _Connection contract below: destruction never
        // releases (safe), an explicit release into a dead sigh faults (the prevented crash).
        // Test-only; adds no public surface.
        friend struct FSignalDelegate_TeardownSpecAccess;
#endif

        CK_GENERATED_BODY(TFragment_Signal_Delegate<T_DynamicDelegate COMMA T_PostFireBehavior COMMA T_Args...>);

    public:
        using ConnectionType = entt::connection;
        using DynamicDelegateType = T_DynamicDelegate;
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
        TFragment_Signal_Delegate() = default;
        TFragment_Signal_Delegate(const ThisType&) = delete;
        TFragment_Signal_Delegate(ThisType&& InOther) noexcept = delete;

    public:
        auto operator=(ThisType InOther) -> ThisType& = delete;
        auto operator=(ThisType&& InOther) noexcept = delete;

    private:
        auto DoBroadcast(T_Args&&... InArgs) -> void;
        auto DoGet_IsBound() const -> bool;
        auto DoAddDelegate(
            DynamicDelegateType InDelegate,
            const DynamicDelegateInvocationPredicateFunc& InOptionalConditionalInvocationPredicate) -> void;
        auto DoRemoveDelegate(
            DynamicDelegateType InDelegate) -> void;

    private:
        TArray<DynamicDelegateType> _UnconditionalDelegates;
        TArray<ConditionalDynamicDelegate> _ConditionalInvocationList;

        /**
         * NON-OWNING token — deliberately no RAII release, and this type must never grow a
         * destructor that calls _Connection.release(). The sigh (in TFragment_Signal, a different
         * pool) owns the subscription; on every whole-entity/whole-registry destruction the sigh
         * dies in the same operation, and during ~basic_registry it dies FIRST, so a destructor-side
         * release is a use-after-free (the packaged save/load crash). Release is required exactly
         * when this fragment is removed from a LIVE entity whose sigh survives — Unbind is that
         * path and releases before Remove (CkSignal_Utils.inl.h). Any new removal path must do the
         * same or the sigh keeps a dangling pointer to this fragment.
         */
        ConnectionType _Connection;
        static constexpr auto PostFireBehavior = T_PostFireBehavior;

        CK_PROPERTY(_Connection);
        CK_PROPERTY(_UnconditionalDelegates);
        CK_PROPERTY(_ConditionalInvocationList);
    };
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkSignal_Fragment.inl.h"

// --------------------------------------------------------------------------------------------------------------------

